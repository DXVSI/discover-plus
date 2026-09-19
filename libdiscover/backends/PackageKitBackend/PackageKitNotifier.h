/*
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <BackendNotifierModule.h>
#include <PackageKit/Transaction>
#include <QPointer>
#include <QVariantList>
#include <functional>

class QTimer;
class QProcess;

class PackageKitNotifier : public BackendNotifierModule
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DISCOVER_NOTIFIER_IID)
    Q_INTERFACES(BackendNotifierModule)
public:
    explicit PackageKitNotifier(QObject *parent = nullptr);
    ~PackageKitNotifier() override;

    bool hasUpdates() override;
    bool hasSecurityUpdates() override;
    void recheckSystemUpdateNeeded() override;
    void refreshDatabase();
    bool needsReboot() const override
    {
        return m_needsReboot;
    }
    void checkDistroUpgrade();

private Q_SLOTS:
    void package(PackageKit::Transaction::Info info, const QString &packageID, const QString &summary);
    void finished(PackageKit::Transaction::Exit exit, uint);
    void onRequireRestart(PackageKit::Transaction::Restart type, const QString &packageID);
    void transactionListChanged(const QStringList &transactionIDs);
    void checkNeedsReboot();

private:
    void nowNeedsReboot();
    void recheckSystemUpdate();
    void checkOfflineUpdates();
    void setupGetUpdatesTransaction(PackageKit::Transaction *transaction);
    QProcess *checkAptVariable(const QString &aptconfig, const QLatin1String &varname, const std::function<void(const QStringView &val)> &func);

    bool m_needsReboot = false;
    uint m_securityUpdates;
    uint m_normalUpdates;
    bool m_hasDistUpgrade;
    QPointer<PackageKit::Transaction> m_refresher;
    QTimer *m_recheckTimer;

    QHash<QString, PackageKit::Transaction *> m_transactions;
};
