/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "updates.h"

#include <QFile>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <discoverdata.h>

#include <updatesdata.h>

K_PLUGIN_FACTORY_WITH_JSON(UpdatesFactory, "kcm_updates.json", registerPlugin<Updates>(); registerPlugin<UpdatesData>();)

Updates::Updates(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new UpdatesData(this))
    , m_discoverData(new DiscoverData(this))
{
    qmlRegisterAnonymousType<UpdatesSettings>("org.kde.discover.updates", 1);
    qmlRegisterAnonymousType<DiscoverSettings>("org.kde.discover.updates", 1);
}

UpdatesSettings *Updates::updatesSettings() const
{
    return m_data->settings();
}

DiscoverSettings *Updates::discoverSettings() const
{
    return m_discoverData->settings();
}

bool Updates::mandatoryRebootAfterUpdate() const
{
#if defined(WITH_SYSUPDATE_BACKEND) || defined(WITH_HOLO_BACKEND)
    return true;
#elif defined WITH_OSTREE_BACKEND
    return QFile::exists(QStringLiteral("/run/ostree-booted"));
#else
    return false;
#endif
}

#include "moc_updates.cpp"
#include "updates.moc"
