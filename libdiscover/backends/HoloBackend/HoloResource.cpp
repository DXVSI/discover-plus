/*
 *   SPDX-FileCopyrightText: 2022 Jeremy Whiting <jeremy.whiting@collabora.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "HoloResource.h"
#include <KLocalizedString>
#include <QDesktopServices>
#include <QStringList>
#include <QTimer>
#include <Transaction/AddonList.h>

HoloResource::HoloResource(const QString &version,
                           const QString &name,
                           const QString &build,
                           quint64 size,
                           const QString &currentVersion,
                           AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_name(name)
    , m_build(build)
    , m_version(version)
    , m_currentVersion(currentVersion)
    , m_appstreamId(QLatin1String("holo.") + m_build)
    , m_state(State::Upgradeable)
    , m_addons()
    , m_type(AbstractResource::System)
    , m_size(size)
{
}

QString HoloResource::appstreamId() const
{
    return m_appstreamId;
}

QList<PackageState> HoloResource::addonsInformation()
{
    return m_addons;
}

QString HoloResource::availableVersion() const
{
    return QStringLiteral("%1 - %2").arg(m_version, m_build);
}

bool HoloResource::hasCategory(const QString &category) const
{
    return QStringLiteral("holo") == category;
}

QString HoloResource::comment()
{
    return name();
}

quint64 HoloResource::size()
{
    return m_size;
}

QUrl HoloResource::homepage()
{
    return QUrl(QStringLiteral("https://store.steampowered.com/"));
}

QUrl HoloResource::helpURL()
{
    return QUrl(QStringLiteral("https://store.steampowered.com/"));
}

QUrl HoloResource::bugURL()
{
    return QUrl(QStringLiteral("https://steamcommunity.com/app/1675200/discussions/"));
}

QUrl HoloResource::donationURL()
{
    return {};
}

QUrl HoloResource::contributeURL()
{
    return {};
}

QVariant HoloResource::icon() const
{
    return QStringLiteral("steam");
}

QString HoloResource::installedVersion() const
{
    return m_currentVersion;
}

QJsonArray HoloResource::licenses()
{
    return {};
}

QString HoloResource::longDescription()
{
    return {};
}

QString HoloResource::name() const
{
    return m_name;
}

QString HoloResource::origin() const
{
    return QStringLiteral("Holo");
}

QString HoloResource::packageName() const
{
    return m_name;
}

bool HoloResource::isRemovable() const
{
    return false;
}

AbstractResource::Type HoloResource::type() const
{
    return m_type;
}

bool HoloResource::canExecute() const
{
    return false;
}

QString HoloResource::section()
{
    return QStringLiteral("Holo");
}

AbstractResource::State HoloResource::state()
{
    return m_state;
}

void HoloResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    Q_EMIT changelogFetched(log);
}

void HoloResource::setState(AbstractResource::State state)
{
    if (m_state == state)
        return;

    m_state = state;
    Q_EMIT stateChanged();
}

void HoloResource::setSize(quint64 size)
{
    m_size = size;
    Q_EMIT sizeChanged();
}

QString HoloResource::sourceIcon() const
{
    return QStringLiteral("steam");
}

QDate HoloResource::releaseDate() const
{
    return {};
}

void HoloResource::setVersion(const QString &version)
{
    m_version = version;
}

void HoloResource::setBuild(const QString &build)
{
    m_build = build;
    m_appstreamId = QLatin1String("holo.") + m_build;
}

QString HoloResource::getBuild() const
{
    return m_build;
}

QUrl HoloResource::url() const
{
    return QUrl(QLatin1String("holo://") + packageName().replace(QLatin1Char(' '), QLatin1Char('.')));
}

QString HoloResource::author() const
{
    return QStringLiteral("Valve");
}

#include "moc_HoloResource.cpp"
