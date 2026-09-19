/*
 *   SPDX-FileCopyrightText: 2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "CoprInstalledPackages.h"
#include "CoprClient.h"
#include "libdiscover_backend_packagekit_debug.h"

#include <QProcessEnvironment>
#include <QTimer>

CoprInstalledPackages::CoprInstalledPackages(QObject *parent)
    : QObject(parent)
{
}

void CoprInstalledPackages::refresh()
{
    m_current = false;
    if (m_process) {
        m_repeat = true;
        return;
    }
    startQuery();
}

void CoprInstalledPackages::startQuery()
{
    m_repeat = false;
    m_process = new QProcess(this);

    // The markers of dnf ("<unknown>") are matched below
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_process->setProcessEnvironment(environment);

    m_queryTimer.start();
    QProcess *process = m_process;
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        const bool succeeded = exitStatus == QProcess::NormalExit && exitCode == 0;
        if (succeeded) {
            setQueryOutput(process->readAllStandardOutput());
        } else {
            qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Could not list the installed packages:" << process->program() << process->arguments()
                                                          << "exit code" << exitCode << process->readAllStandardError().trimmed();
        }
        qCDebug(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Listed the installed packages of" << m_byRepository.size() << "COPR repositories in"
                                                    << m_queryTimer.elapsed() << "ms";
        queryFinished(succeeded);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        // Everything else is followed by finished()
        if (error != QProcess::FailedToStart) {
            return;
        }
        qCWarning(LIBDISCOVER_BACKEND_PACKAGEKIT_LOG) << "Could not start" << process->program() << "to list the installed packages";
        queryFinished(false);
    });
    QTimer::singleShot(QueryTimeoutMs, process, &QProcess::kill);

    // Nothing is downloaded for --installed; a real tab, dnf does not know "\t"
    m_process->start(QStringLiteral("dnf"),
                     {QStringLiteral("--quiet"),
                      QStringLiteral("repoquery"),
                      QStringLiteral("--installed"),
                      QStringLiteral("--queryformat"),
                      QStringLiteral("%{name}\t%{evr}\t%{from_repo}\t%{vendor}\n")});
}

void CoprInstalledPackages::queryFinished(bool succeeded)
{
    m_process->deleteLater();
    m_process = nullptr;

    if (m_repeat) {
        startQuery();
        return;
    }
    if (!succeeded) {
        m_byRepository.clear();
        m_byVendor.clear();
    }
    m_known = succeeded;
    m_current = true;
    Q_EMIT refreshed();
}

void CoprInstalledPackages::setQueryOutput(const QByteArray &output)
{
    m_byRepository.clear();
    m_byVendor.clear();

    const QString userVendor = QStringLiteral("Fedora Copr - user ");
    const QString groupVendor = QStringLiteral("Fedora Copr - group ");

    const QStringList lines = QString::fromUtf8(output).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList fields = line.split(QLatin1Char('\t'));
        if (fields.size() != 4) {
            continue;
        }
        const QString &name = fields.at(0);
        const QString &version = fields.at(1);
        const QString &repository = fields.at(2);
        const QString &vendor = fields.at(3);

        if (repository.startsWith(QLatin1String("copr:"))) {
            m_byRepository[repository].insert(name, version);
            continue;
        }
        // Only without a record: a package that dnf installed from Fedora is never claimed.
        // "@commandline" and the like are not repositories either.
        const bool hasRepositoryRecord = !repository.isEmpty() && repository != QLatin1String("<unknown>") && !repository.startsWith(QLatin1Char('@'));
        if (hasRepositoryRecord) {
            continue;
        }
        if (vendor.startsWith(userVendor)) {
            m_byVendor[vendor.mid(userVendor.size())].insert(name, version);
        } else if (vendor.startsWith(groupVendor)) {
            const QString group = vendor.mid(groupVendor.size());
            m_byVendor[group.startsWith(QLatin1Char('@')) ? group : QLatin1Char('@') + group].insert(name, version);
        }
    }
}

QString CoprInstalledPackages::installedVersion(const QString &owner, const QString &project, const QString &packageName) const
{
    const QString repositoryId = CoprClient::repositoryId(owner, project);
    if (repositoryId.isEmpty() || packageName.isEmpty()) {
        return {};
    }

    // The ":ml" repository of the same project carries its multilib packages
    for (const QString &id : {repositoryId, QString(repositoryId + QLatin1String(":ml"))}) {
        const QString version = m_byRepository.value(id).value(packageName);
        if (!version.isEmpty()) {
            return version;
        }
    }
    return m_byVendor.value(owner).value(packageName);
}

QStringList CoprInstalledPackages::packagesFromRepository(const QString &owner, const QString &project) const
{
    const QString repositoryId = CoprClient::repositoryId(owner, project);
    if (repositoryId.isEmpty()) {
        return {};
    }

    QStringList packages = m_byRepository.value(repositoryId).keys();
    packages += m_byRepository.value(repositoryId + QLatin1String(":ml")).keys();
    packages.removeDuplicates();
    packages.sort();
    return packages;
}
