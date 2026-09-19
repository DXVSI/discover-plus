/*
 *   SPDX-FileCopyrightText: 2026 DXVSI <https://github.com/DXVSI>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COPRINSTALLEDPACKAGES_H
#define COPRINSTALLEDPACKAGES_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>

// Which installed packages came from which COPR repository. One unprivileged
// "dnf repoquery --installed" answers for all of them (about 0.3 s for 4000 packages):
// dnf records the repository every package was installed from.
class CoprInstalledPackages : public QObject
{
    Q_OBJECT
public:
    explicit CoprInstalledPackages(QObject *parent = nullptr);

    // False until a query has answered, and again from refresh() until the next answer
    bool isCurrent() const
    {
        return m_current;
    }
    bool isRunning() const
    {
        return m_process;
    }
    // False when the last query failed: nothing is known then, which is not "nothing is installed"
    bool isKnown() const
    {
        return m_known;
    }
    // Asks the system (again). A query that is already running is repeated when it ends:
    // what it saw may be older than the reason for this call. refreshed() follows.
    void refresh();

    // Empty when the package is not installed from the repository of this project. Group
    // owners are "@group" here and "group_group" in a repository id, both are the same.
    // A package without a record of its repository (installed with rpm, from a file, or
    // before dnf kept the record) is matched by its vendor instead, "Fedora Copr - user
    // <owner>" or "Fedora Copr - group @<group>": that names the owner but not the project.
    QString installedVersion(const QString &owner, const QString &project, const QString &packageName) const;
    // The installed packages that came from the repository of this project, by that record only
    QStringList packagesFromRepository(const QString &owner, const QString &project) const;
    // The installed packages of this owner that are known by their vendor only: any of them
    // may have come from any project of the owner
    QStringList packagesWithoutRepositoryRecord(const QString &owner) const;

    // Lines of "name<TAB>evr<TAB>from_repo<TAB>vendor"
    void setQueryOutput(const QByteArray &output);

Q_SIGNALS:
    void refreshed();

private:
    void startQuery();
    void queryFinished(bool succeeded);

    QPointer<QProcess> m_process;
    QElapsedTimer m_queryTimer;
    bool m_repeat = false;
    bool m_current = false;
    bool m_known = false;
    // Repository id -> package name -> version. Exactly as written: COPR tells "Proj" from "proj", and so does dnf
    QHash<QString, QHash<QString, QString>> m_byRepository;
    // Owner as the API writes it -> package name -> version, for packages without a repository record
    QHash<QString, QHash<QString, QString>> m_byVendor;

    static constexpr int QueryTimeoutMs = 60000;
};

#endif // COPRINSTALLEDPACKAGES_H
