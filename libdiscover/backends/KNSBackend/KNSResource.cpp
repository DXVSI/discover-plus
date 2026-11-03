/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSResource.h"
#include "KNSBackend.h"
#include <KLocalizedString>
#include <KNSCore/EngineBase>
#include <KNSCore/Transaction>
#include <KShell>
#include <QProcess>
#include <QRegularExpression>

#include "ReviewsBackend/Rating.h"
#include <appstream/AppStreamUtils.h>
#include <attica/provider.h>
#include <utils.h>

KNSResource::KNSResource(const KNSCore::Entry &entry, QStringList categories, KNSBackend *parent)
    : AbstractResource(parent)
    , m_categories(std::move(categories))
    , m_entry(entry)
    , m_lastStatus(entry.status())
{
    connect(this, &KNSResource::stateChanged, parent, &KNSBackend::updatesCountChanged);
}

KNSResource::~KNSResource() = default;

AbstractResource::State KNSResource::state()
{
    switch (m_entry.status()) {
    case KNSCore::Entry::Invalid:
        return Broken;
    case KNSCore::Entry::Downloadable:
        return None;
    case KNSCore::Entry::Installed:
        return Installed;
    case KNSCore::Entry::Updateable:
        return Upgradeable;
    case KNSCore::Entry::Deleted:
    case KNSCore::Entry::Installing:
    case KNSCore::Entry::Updating:
        return None;
    }
    return None;
}

KNSBackend *KNSResource::knsBackend() const
{
    return qobject_cast<KNSBackend *>(parent());
}

QVariant KNSResource::icon() const
{
    const QString thumbnail = m_entry.previewUrl(KNSCore::Entry::PreviewSmall1);
    return thumbnail.isEmpty() ? knsBackend()->iconName() : m_entry.previewUrl(KNSCore::Entry::PreviewSmall1);
}

QString KNSResource::comment()
{
    QString ret = m_entry.shortSummary();
    if (ret.isEmpty()) {
        ret = m_entry.summary();
        int newLine = ret.indexOf(QLatin1Char('\n'));
        if (newLine > 0) {
            ret.truncate(newLine);
        }
        ret.remove(QRegularExpression(QStringLiteral("\\[\\/?[a-z]*\\]")));
        ret.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    }
    return ret;
}

QString KNSResource::longDescription()
{
    QString ret = m_entry.summary();
    if (m_entry.shortSummary().isEmpty()) {
        const int newLine = ret.indexOf(QLatin1Char('\n'));
        if (newLine < 0)
            ret.clear();
        else
            ret = ret.mid(newLine + 1).trimmed();
    }
    ret.remove(QLatin1Char('\r'));
    ret.replace(QStringLiteral("[li]"), QStringLiteral("\n* "));
    // Get rid of all BBCode markup we don't handle above
    ret.remove(QRegularExpression(QStringLiteral("\\[\\/?[a-z]*\\]")));
    // Find anything that looks like a link (but which also is not some html
    // tag value or another already) and make it a link
    static const QRegularExpression urlRegExp(
        QStringLiteral("(^|\\s)(http[-a-zA-Z0-9@:%_\\+.~#?&//=]{2,256}\\.[a-z]{2,4}\\b(\\/[-a-zA-Z0-9@:;%_\\+.~#?&//=]*)?)"),
        QRegularExpression::CaseInsensitiveOption);
    ret.replace(urlRegExp, QStringLiteral("<a href=\"\\2\">\\2</a>"));
    return ret;
}

QString KNSResource::name() const
{
    return m_entry.name();
}

QString KNSResource::packageName() const
{
    return m_entry.uniqueId();
}

bool KNSResource::hasCategory(const QString &category) const
{
    return m_categories.contains(category);
}

QUrl KNSResource::homepage()
{
    return m_entry.homepage();
}

void KNSResource::setEntry(const KNSCore::Entry &entry)
{
    const bool diff = entry.status() != m_lastStatus;
    m_entry = entry;
    if (diff) {
        m_lastStatus = entry.status();
        Q_EMIT stateChanged();
    }
}

KNSCore::Entry KNSResource::entry() const
{
    return m_entry;
}

QJsonArray KNSResource::licenses()
{
    return {{AppStreamUtils::license(m_entry.license())}};
}

quint64 KNSResource::size()
{
    const auto downloadInfo = m_entry.downloadLinkInformationList();
    return downloadInfo.isEmpty() ? 0 : downloadInfo.at(0).size * 1024;
}

QString KNSResource::installedVersion() const
{
    return !m_entry.version().isEmpty() ? m_entry.version() : m_entry.releaseDate().toString();
}

QString KNSResource::availableVersion() const
{
    return !m_entry.updateVersion().isEmpty()   ? m_entry.updateVersion()
        : !m_entry.updateReleaseDate().isNull() ? m_entry.updateReleaseDate().toString()
        : !m_entry.version().isEmpty()          ? m_entry.version()
                                                : releaseDate().toString();
}

QString KNSResource::origin() const
{
    return m_entry.providerId();
}

QString KNSResource::displayOrigin() const
{
    if (auto providers = knsBackend()->engine()->atticaProviders(); !providers.isEmpty()) {
        auto provider = providers.constFirst();
        if (provider->name() == QLatin1String("api.kde-look.org")) {
            return i18nc("The name of the KDE Store", "KDE Store");
        }
        return providers.constFirst()->name();
    }
    return QUrl(m_entry.providerId()).host();
}

QString KNSResource::section()
{
    return m_entry.category();
}

static bool isAnimated(const QString &path)
{
    static const QVector<QLatin1String> s_extensions = {QLatin1String(".gif"), QLatin1String(".apng"), QLatin1String(".webp"), QLatin1String(".avif")};
    return std::ranges::any_of(s_extensions, [path](const QLatin1String &postfix) {
        return path.endsWith(postfix);
    });
}

static void appendIfValid(Screenshots &list, const QUrl &thumbnail, const QUrl &screenshot)
{
    if (thumbnail.isEmpty() || screenshot.isEmpty()) {
        return;
    }
    list += {thumbnail, screenshot, isAnimated(thumbnail.path()), {}};
}

void KNSResource::fetchScreenshots()
{
    Screenshots ret;
    appendIfValid(ret, QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewSmall1)), QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewBig1)));
    appendIfValid(ret, QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewSmall2)), QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewBig2)));
    appendIfValid(ret, QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewSmall3)), QUrl(m_entry.previewUrl(KNSCore::Entry::PreviewBig3)));
    Q_EMIT screenshotsFetched(ret);
}

void KNSResource::fetchChangelog()
{
    Q_EMIT changelogFetched(m_entry.changelog());
}

QStringList KNSResource::extends() const
{
    return knsBackend()->extends();
}

QUrl KNSResource::url() const
{
    return QUrl(QStringLiteral("kns://") + knsBackend()->name() + QLatin1Char('/') + m_entry.uniqueId());
}

bool KNSResource::canExecute() const
{
    return knsBackend()->engine()->hasAdoptionCommand();
}

void KNSResource::invokeApplication() const
{
    KNSCore::Transaction::adopt(knsBackend()->engine(), m_entry);
}

QString KNSResource::executeLabel() const
{
    return knsBackend()->engine()->useLabel();
}

QDate KNSResource::releaseDate() const
{
    return m_entry.updateReleaseDate().isNull() ? m_entry.releaseDate() : m_entry.updateReleaseDate();
}

QVector<int> KNSResource::linkIds() const
{
    QVector<int> ids;
    const auto linkInfo = m_entry.downloadLinkInformationList();
    for (const auto &e : linkInfo) {
        if (e.isDownloadtypeLink)
            ids << e.id;
    }
    return ids;
}

QUrl KNSResource::donationURL()
{
    return QUrl(m_entry.donationLink());
}

Rating KNSResource::rating() const
{
    if (!m_rating.has_value()) {
        const int noc = m_entry.numberOfComments();
        const int rating = m_entry.rating();
        Q_ASSERT(rating <= 100);
        m_rating = Rating(packageName(), noc, rating / 10);
    }
    return m_rating.value();
}

QString KNSResource::author() const
{
    return m_entry.author().name();
}

#include "moc_KNSResource.cpp"
