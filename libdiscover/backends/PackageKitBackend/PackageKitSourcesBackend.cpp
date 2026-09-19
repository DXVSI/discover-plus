/*
 *   SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitSourcesBackend.h"
#include "PackageKitBackend.h"
#include "config-paths.h"
#include <KDesktopFile>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <resources/AbstractResourcesBackend.h>
#include <resources/DiscoverAction.h>
#include <resources/SourcesModel.h>

class PKSourcesModel : public QStandardItemModel
{
public:
    explicit PKSourcesModel(PackageKitSourcesBackend *backend)
        : QStandardItemModel(backend)
        , m_backend(backend)
    {
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        auto item = itemFromIndex(index);
        if (!item) {
            return false;
        }

        switch (role) {
        case Qt::CheckStateRole: {
            auto transaction = PackageKit::Daemon::global()->repoEnable(item->data(AbstractSourcesBackend::IdRole).toString(), value.toInt() == Qt::Checked);
            connect(transaction, &PackageKit::Transaction::errorCode, m_backend, &PackageKitSourcesBackend::transactionError);
            return true;
        }
        }
        item->setData(value, role);
        return true;
    }

private:
    PackageKitSourcesBackend *m_backend;
};

static DiscoverAction *createActionForService(const QString &servicePath, PackageKitSourcesBackend *backend)
{
    DiscoverAction *action = new DiscoverAction(backend);
    KDesktopFile parser(servicePath);
    action->setIconName(parser.readIcon());
    action->setText(parser.readName());
    action->setToolTip(parser.readComment());
    QObject::connect(action, &DiscoverAction::triggered, action, [backend, servicePath]() {
        KService::Ptr service = KService::serviceByStorageId(servicePath);
        if (!service) {
            qWarning() << "Failed to find service" << servicePath;
            return;
        }

        auto *job = new KIO::ApplicationLauncherJob(service);
        QObject::connect(job, &KJob::finished, backend, [backend, service](KJob *job) {
            if (job->error()) {
                Q_EMIT backend->passiveMessage(i18n("Failed to start “%1”: %2", service->name(), job->errorString()));
            }
        });
        job->start();
    });
    return action;
}

PackageKitSourcesBackend::PackageKitSourcesBackend(AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_sources(new PKSourcesModel(this))
{
    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::repoListChanged, this, &PackageKitSourcesBackend::resetSources);
    connect(SourcesModel::global(), &SourcesModel::showingNow, this, &PackageKitSourcesBackend::resetSources);

    // Kubuntu-based
    auto addNativeSourcesManager = [this](const QString &file) {
        auto service = PackageKitBackend::locateService(file);
        if (!service.isEmpty()) {
            m_actions += QVariant::fromValue<QObject *>(createActionForService(service, this));
        }
    };

    // New Ubuntu
    addNativeSourcesManager(QStringLiteral("software-properties-qt.desktop"));

    // Old Ubuntu
    addNativeSourcesManager(QStringLiteral("software-properties-kde.desktop"));

    // OpenSuse
    addNativeSourcesManager(QStringLiteral("YaST2/sw_source.desktop"));
}

QString PackageKitSourcesBackend::idDescription()
{
    return i18n("Repository URL:");
}

QStandardItem *PackageKitSourcesBackend::findItemForId(const QString &id) const
{
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(AbstractSourcesBackend::IdRole).toString() == id) {
            return it;
        }
    }
    return nullptr;
}

void PackageKitSourcesBackend::addRepositoryDetails(const QString &id, const QString &description, bool enabled)
{
    bool add = false;
    QStandardItem *item = findItemForId(id);

    if (!item) {
        item = new QStandardItem(description);
        if (PackageKit::Daemon::backendName() == QLatin1String("aptcc")) {
            QRegularExpression exp(QStringLiteral("^/etc/apt/sources.list.d/(.+?).list:.*"));

            auto matchIt = exp.globalMatch(id);
            if (matchIt.hasNext()) {
                auto match = matchIt.next();
                item->setData(match.captured(1), Qt::ToolTipRole);
            }
        }
        item->setCheckable(PackageKit::Daemon::roles() & PackageKit::Transaction::RoleRepoEnable);
        add = true;
    }
    item->setData(id, IdRole);
    item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
    item->setEnabled(true);

    if (add) {
        m_sources->appendRow(item);
    }
}

QAbstractItemModel *PackageKitSourcesBackend::sources()
{
    return m_sources;
}

bool PackageKitSourcesBackend::addSource(const QString & /*id*/)
{
    return false;
}

bool PackageKitSourcesBackend::removeSource(const QString &id)
{
    auto transaction = PackageKit::Daemon::global()->repoRemove(id, false);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    return false;
}

QVariantList PackageKitSourcesBackend::actions() const
{
    return m_actions;
}

void PackageKitSourcesBackend::resetSources()
{
    disconnect(SourcesModel::global(), &SourcesModel::showingNow, this, &PackageKitSourcesBackend::resetSources);
    for (int i = 0, count = m_sources->rowCount(); i < count; ++i) {
        m_sources->item(i, 0)->setEnabled(false);
    }
    auto transaction = PackageKit::Daemon::global()->getRepoList();
    connect(transaction, &PackageKit::Transaction::repoDetail, this, &PackageKitSourcesBackend::addRepositoryDetails);
    connect(transaction, &PackageKit::Transaction::errorCode, this, &PackageKitSourcesBackend::transactionError);
    connect(transaction, &PackageKit::Transaction::finished, this, [this] {
        for (int i = 0; i < m_sources->rowCount();) {
            if (!m_sources->item(i, 0)->isEnabled()) {
                m_sources->removeRow(i);
            } else {
                ++i;
            }
        }
    });
}

void PackageKitSourcesBackend::transactionError(PackageKit::Transaction::Error error, const QString &message)
{
    Q_EMIT passiveMessage(message);
    qWarning() << "Transaction error: " << error << message << sender();
}

#include "moc_PackageKitSourcesBackend.cpp"
