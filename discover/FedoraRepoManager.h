/*
 *   SPDX-FileCopyrightText: 2025 Discover Plus Contributors
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QQmlEngine>

class QProcess;

class FedoraRepoManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isFedora READ isFedora CONSTANT)
    Q_PROPERTY(bool rpmFusionFreeInstalled READ rpmFusionFreeInstalled NOTIFY statusChanged)
    Q_PROPERTY(bool rpmFusionNonfreeInstalled READ rpmFusionNonfreeInstalled NOTIFY statusChanged)
    Q_PROPERTY(bool rpmFusionFreeAppstreamInstalled READ rpmFusionFreeAppstreamInstalled NOTIFY statusChanged)
    Q_PROPERTY(bool rpmFusionNonfreeAppstreamInstalled READ rpmFusionNonfreeAppstreamInstalled NOTIFY statusChanged)
    Q_PROPERTY(bool flathubInstalled READ flathubInstalled NOTIFY statusChanged)
    Q_PROPERTY(bool dnfConfigured READ dnfConfigured NOTIFY statusChanged)
    Q_PROPERTY(bool ciscoRepoEnabled READ ciscoRepoEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool googleChromeRepoEnabled READ googleChromeRepoEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool nvidiaRepoEnabled READ nvidiaRepoEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool steamRepoEnabled READ steamRepoEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool setupNeeded READ setupNeeded NOTIFY statusChanged)
    Q_PROPERTY(bool firstRunCompleted READ firstRunCompleted WRITE setFirstRunCompleted NOTIFY firstRunCompletedChanged)
    Q_PROPERTY(bool installing READ installing NOTIFY installingChanged)
    Q_PROPERTY(QString installError READ installError NOTIFY installErrorChanged)

public:
    static FedoraRepoManager *create(QQmlEngine *engine, QJSEngine *);
    static FedoraRepoManager *instance();

    bool isFedora() const;
    bool rpmFusionFreeInstalled() const;
    bool rpmFusionNonfreeInstalled() const;
    bool rpmFusionFreeAppstreamInstalled() const;
    bool rpmFusionNonfreeAppstreamInstalled() const;
    bool flathubInstalled() const;
    bool dnfConfigured() const;
    bool ciscoRepoEnabled() const;
    bool googleChromeRepoEnabled() const;
    bool nvidiaRepoEnabled() const;
    bool steamRepoEnabled() const;
    bool setupNeeded() const;
    bool firstRunCompleted() const;
    void setFirstRunCompleted(bool completed);
    bool installing() const;
    QString installError() const;

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void installRpmFusion(bool free, bool nonfree, bool appstreamData);
    Q_INVOKABLE void installFlathub();
    Q_INVOKABLE void applySetup(bool configureDnf,
                                bool disableCisco,
                                bool rpmFusionFree,
                                bool rpmFusionNonfree,
                                bool flathub,
                                bool enableChrome,
                                bool enableNvidia,
                                bool enableSteam);

Q_SIGNALS:
    void statusChanged();
    void firstRunCompletedChanged();
    void installingChanged();
    void installErrorChanged();
    void installationFinished(bool success);

private:
    explicit FedoraRepoManager(QObject *parent = nullptr);

    void checkPackageInstalled(const QString &packageName, bool &result);
    void runDnfInstall(const QStringList &packages);
    bool isRepoEnabled(const QString &repoId) const;

    bool m_isFedora = false;
    bool m_rpmFusionFreeInstalled = false;
    bool m_rpmFusionNonfreeInstalled = false;
    bool m_rpmFusionFreeAppstreamInstalled = false;
    bool m_rpmFusionNonfreeAppstreamInstalled = false;
    bool m_flathubInstalled = false;
    bool m_dnfConfigured = false;
    bool m_ciscoRepoEnabled = true;
    bool m_googleChromeRepoEnabled = false;
    bool m_nvidiaRepoEnabled = false;
    bool m_steamRepoEnabled = false;
    bool m_firstRunCompleted = false;
    bool m_installing = false;
    QString m_installError;

    static FedoraRepoManager *s_instance;
};
