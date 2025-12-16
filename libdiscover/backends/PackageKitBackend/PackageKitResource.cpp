/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2013 Lukas Appelhans <l.appelhans@gmx.de>
 *   SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "PackageKitResource.h"
#include "PackageKitBackend.h"
#include "PackageKitMessages.h"
#include "appstream/AppStreamUtils.h"
#include "config-paths.h"
#include <AppStreamQt/spdx.h>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KService>
#include <KShell>
#include <PackageKit/Daemon>
#include <QDebug>
#include <QIcon>
#include <QJsonArray>
#include <QProcess>
#include <appstream/AppStreamIntegration.h>
#include <utils.h>

#if defined(WITH_MARKDOWN)
extern "C" {
#include <mkdio.h>
}
#endif

using namespace Qt::StringLiterals;

const QStringList PackageKitResource::s_topObjects({QStringLiteral("qrc:/qml/DependenciesButton.qml")});
const QStringList PackageKitResource::s_bottomObjects({QStringLiteral("qrc:/qml/PackageKitPermissions.qml")});

PackageKitResource::PackageKitResource(QString packageName, QString summary, PackageKitBackend *parent)
    : AbstractResource(parent)
    , m_summary(std::move(summary))
    , m_name(std::move(packageName))
{
    setObjectName(m_name);
    connect(this, &AbstractResource::stateChanged, &m_dependencies, &PackageKitDependencies::setDirty);
    connect(&m_dependencies, &PackageKitDependencies::dependenciesChanged, this, [this] {
        Q_EMIT dependenciesChanged();
        Q_EMIT sizeChanged();
    });
}

QString PackageKitResource::name() const
{
    return m_name;
}

QString PackageKitResource::packageName() const
{
    return m_name;
}

QStringList PackageKitResource::allPackageNames() const
{
    return {m_name};
}

QString PackageKitResource::availablePackageId() const
{
    // First we check if it's upgradeable and use this version to display
    const QSet<QString> pkgids = backend()->upgradeablePackageId(this);
    if (!pkgids.isEmpty()) {
        return *pkgids.constBegin();
    }

    const auto it = m_packages.constFind(PackageKit::Transaction::InfoAvailable);
    if (it != m_packages.constEnd()) {
        return it->first();
    }
    return installedPackageId();
}

QString PackageKitResource::installedPackageId() const
{
    const auto installed = m_packages[PackageKit::Transaction::InfoInstalled];
    return installed.isEmpty() ? QString() : installed.first();
}

QString PackageKitResource::comment()
{
    return m_summary;
}

QString PackageKitResource::longDescription()
{
    fetchDetails();
    return m_details.description();
}

QUrl PackageKitResource::homepage()
{
    fetchDetails();
    return QUrl(m_details.url());
}

QVariant PackageKitResource::icon() const
{
    const QString pkgName = packageName();
    if (pkgName.isEmpty()) {
        return QStringLiteral("applications-other");
    }

    // Try package name directly (works for discord, firefox, etc.)
    if (QIcon::hasThemeIcon(pkgName)) {
        return pkgName;
    }

    // Try org.package.name format (for telegram-desktop -> org.telegram.desktop)
    QString orgName = QStringLiteral("org.") + QString(pkgName).replace(QLatin1Char('-'), QLatin1Char('.'));
    if (QIcon::hasThemeIcon(orgName)) {
        return orgName;
    }

    // Fallback to package name anyway (might resolve later)
    return pkgName;
}

static QMap<QString, QString> s_translation = {
    {u"AGPL"_s, u"AGPL-3.0"_s}, //
    {u"AGPL3"_s, u"AGPL-3.0"_s}, //
    {u"Artistic2.0"_s, u"Artistic-2.0"_s}, //
    {u"Apache"_s, u"Apache-2.0"_s}, //
    {u"APACHE"_s, u"Apache-2.0"_s}, //
    {u"CCPL"_s, u"CC0-1.0"_s}, //
    {u"GPL2"_s, u"GPL-2.0"_s}, //
    {u"GPL3"_s, u"GPL-3.0"_s}, //
    {u"FDL1.2"_s, u"GFDL-1.2-only"_s}, //
    {u"FDL1.3"_s, u"GFDL-1.3-only"_s}, //
    {u"LGPL"_s, u"LGPL-2.1"_s}, //
    {u"LGPL3"_s, u"LGPL-3.0"_s}, //
    {u"MPL"_s, u"MPL-1.1"_s}, //
    {u"MPL2"_s, u"MPL-2.0"_s}, //
    {u"PerlArtistic"_s, u"Artistic-1.0-Perl"_s}, //
    {u"PHP"_s, u"PHP-3.01"_s}, //
    {u"PSF"_s, u"Python-2.0"_s}, //
    {u"RUBY"_s, u"Ruby"_s}, //
    {u"ZPL"_s, u"ZPL-2.1"_s}, //
};

QJsonArray PackageKitResource::licenses()
{
    fetchDetails();

    if (!m_details.license().isEmpty()) {
        QString id = m_details.license();
        if (!AppStream::SPDX::isLicenseId(id)) {
            auto spdxId = AppStream::SPDX::asSpdxId(id);
            if (!spdxId.isEmpty()) {
                id = spdxId;
            }
        }

        if (!AppStream::SPDX::isLicenseId(id)) {
            id = s_translation.value(id, id);
        }
        return {AppStreamUtils::license(id)};
    }

    return {};
}

QList<PackageState> PackageKitResource::addonsInformation()
{
    return QList<PackageState>();
}

QString PackageKitResource::availableVersion() const
{
    return PackageKit::Daemon::packageVersion(availablePackageId());
}

QString PackageKitResource::installedVersion() const
{
    return PackageKit::Daemon::packageVersion(installedPackageId());
}

quint64 PackageKitResource::size()
{
    fetchDetails();
    return m_details.size();
}

QString PackageKitResource::origin() const
{
    auto osRelease = AppStreamIntegration::global()->osRelease();

    if (PackageKit::Daemon::backendName() == QStringLiteral("apt")) {
        // Debian and its derivatives have a defined scheme for repository origins that we can parse, to
        // guess a better origin name.
        QString pkgid = availablePackageId();
        QString dataField = PackageKit::Daemon::packageData(pkgid);
        // The "data" field of a package-id may contain a modifier such as "auto:" or "manual:", so
        // we will need to strip that in case it exists to extract the actual origin.
        // The data field may look like "auto:debian-bookworm-main" or "google_llc-stable-main",
        // so we can set the OS name if we see it as origin prefix, and otherwise need to fall back
        // to the origin string.
        int i = dataField.indexOf(QLatin1Char(':'));
        QString origin = i > 0 ? dataField.mid(i + 1) : dataField;
        if (origin.startsWith(osRelease->id().toLower() + QLatin1Char('-'))) {
            return osRelease->name();
        } else {
            return origin.isEmpty() ? i18n("Unknown Source") : origin;
        }
    }

    // Extract repository name from package ID
    // PackageID format: name;version;arch;data where data is the repository name
    QString pkgid = availablePackageId().isEmpty() ? installedPackageId() : availablePackageId();
    if (!pkgid.isEmpty()) {
        QString repoName = PackageKit::Daemon::packageData(pkgid);
        if (!repoName.isEmpty()) {
            // Clean up repo name - remove "installed:" prefix if present
            if (repoName.startsWith(QStringLiteral("installed:"))) {
                repoName = repoName.mid(10);
            }

            // Skip if repo name is just "installed" or empty after cleanup
            if (repoName.isEmpty() || repoName == QStringLiteral("installed")) {
                return osRelease->name();
            }

            // Map common repository names to friendly names
            if (repoName == QStringLiteral("fedora")) {
                return osRelease->name(); // Show "Fedora Linux" for main repo
            } else if (repoName.startsWith(QStringLiteral("rpmfusion-"))) {
                return QStringLiteral("RPM Fusion");
            } else if (repoName.startsWith(QStringLiteral("@copr:")) || repoName.startsWith(QStringLiteral("copr:"))) {
                // COPR repos format: @copr:owner:project or copr:owner:project
                return QStringLiteral("COPR");
            } else if (repoName == QStringLiteral("updates") || repoName == QStringLiteral("updates-testing")) {
                return osRelease->name(); // Fedora updates
            } else {
                // Return repository name as-is for other repos (google-chrome, etc.)
                return repoName;
            }
        }
    }

    // Fallback to distro name
    return osRelease->name();
}

QString PackageKitResource::section()
{
    return QString();
}

AbstractResource::State PackageKitResource::state()
{
    if (backend()->isPackageNameUpgradeable(this)) {
        return Upgradeable;
    } else if (m_packages.contains(PackageKit::Transaction::InfoInstalled)) {
        return Installed;
    } else if (m_packages.contains(PackageKit::Transaction::InfoAvailable)) {
        return None;
    } else {
        return Broken;
    }
}

void PackageKitResource::addPackageId(PackageKit::Transaction::Info info, const QString &packageId, bool arch)
{
    auto oldState = state();
    if (arch) {
        m_packages[info].archPkgIds.append(packageId);
    } else {
        m_packages[info].nonarchPkgIds.append(packageId);
    }

    if (oldState != state()) {
        Q_EMIT stateChanged();
    }

    Q_EMIT versionsChanged();
}

bool PackageKitResource::hasCategory(const QString & /*category*/) const
{
    return false;
}

AbstractResource::Type PackageKitResource::type() const
{
    const QString pkgName = packageName();

    // If package has a theme icon, it's an application
    if (!pkgName.isEmpty()) {
        if (QIcon::hasThemeIcon(pkgName)) {
            return Application;
        }
        // Try org.package.name format
        QString orgName = QStringLiteral("org.") + QString(pkgName).replace(QLatin1Char('-'), QLatin1Char('.'));
        if (QIcon::hasThemeIcon(orgName)) {
            return Application;
        }
    }

    // Show RPM Fusion packages as applications (except libs, devel, doc packages)
    QString pkgid = availablePackageId().isEmpty() ? installedPackageId() : availablePackageId();
    if (!pkgid.isEmpty()) {
        QString repoName = PackageKit::Daemon::packageData(pkgid);
        if (repoName.startsWith(QStringLiteral("rpmfusion-"))) {
            // Filter out non-application packages
            if (!pkgName.endsWith(QStringLiteral("-devel")) && !pkgName.endsWith(QStringLiteral("-libs")) && !pkgName.endsWith(QStringLiteral("-lib"))
                && !pkgName.endsWith(QStringLiteral("-doc")) && !pkgName.endsWith(QStringLiteral("-docs")) && !pkgName.endsWith(QStringLiteral("-common"))
                && !pkgName.endsWith(QStringLiteral("-data")) && !pkgName.startsWith(QStringLiteral("lib")) && !pkgName.startsWith(QStringLiteral("kmod-"))
                && !pkgName.startsWith(QStringLiteral("akmod-"))) {
                return Application;
            }
        }
    }
    return System;
}

void PackageKitResource::fetchDetails()
{
    const QString pkgid = availablePackageId();
    if (!m_details.isEmpty() || pkgid.isEmpty()) {
        return;
    }
    m_details.insert(QStringLiteral("fetching"), true); // we add an entry so it's not re-fetched.

    backend()->fetchDetails(pkgid);
}

void PackageKitResource::failedFetchingDetails(PackageKit::Transaction::Error error, const QString &msg)
{
    qWarning() << "error fetching details" << error << msg;
}

QList<PackageKitDependency> PackageKitResource::dependencies()
{
    return m_dependencies.dependencies();
}

void PackageKitResource::setDetails(const PackageKit::Details &details)
{
    const bool ourDetails = details.packageId() == availablePackageId();
    if (!ourDetails) {
        return;
    }

    if (m_details != details) {
        const auto oldState = state();
        const auto oldSize = m_details.size();
        const auto oldLicense = m_details.license();
        const auto oldDescription = m_details.description();
        m_details = details;

        if (oldState != state()) {
            Q_EMIT stateChanged();
        }

        Q_EMIT backend()->resourcesChanged(this, {"size", "homepage", "license"});

        if (oldSize != uint(size())) {
            Q_EMIT sizeChanged();
        }

        if (oldLicense != m_details.license()) {
            Q_EMIT licensesChanged();
        }

        if (oldDescription != m_details.description()) {
            Q_EMIT longDescriptionChanged();
        }
    }
}

void PackageKitResource::fetchChangelog()
{
}

void PackageKitResource::fetchUpdateDetails()
{
    const auto pkgid = availablePackageId();
    if (pkgid.isEmpty()) {
        auto a = new OneTimeAction(
            [this] {
                fetchUpdateDetails();
            },
            this);
        connect(this, &PackageKitResource::stateChanged, a, &OneTimeAction::trigger);
        return;
    }
    backend()->updateDetails().add(pkgid);
}

static void addIfNotEmpty(const QString &title, const QString &content, QString &where)
{
    if (!content.isEmpty()) {
        where += QLatin1String("<p><b>") + title + QLatin1String("</b>&nbsp;") + QString(content).replace(QLatin1Char('\n'), QLatin1String("<br />"))
            + QLatin1String("</p>");
    }
}

QString PackageKitResource::joinPackages(const QStringList &pkgids, const QString &_sep, const QString &shadowPackage)
{
    QStringList ret;
    for (const QString &pkgid : pkgids) {
        const auto pkgname = PackageKit::Daemon::packageName(pkgid);
        if (pkgname == shadowPackage) {
            ret += PackageKit::Daemon::packageVersion(pkgid);
        } else {
            ret += i18nc("package-name (version)", "%1 (%2)", pkgname, PackageKit::Daemon::packageVersion(pkgid));
        }
    }
    const QString sep = _sep.isEmpty() ? i18nc("comma separating package names", ", ") : _sep;
    return ret.join(sep);
}

static QStringList urlToLinks(const QStringList &urls)
{
    QStringList ret;
    for (const auto &url : urls) {
        ret += QStringLiteral("<a href='%1'>%1</a>").arg(url);
    }
    return ret;
}

QString PackageKitResource::verifiedMessage() const
{
    return i18n("Software verified by %1", displayOrigin());
}

bool PackageKitResource::containsPackageId(const QString &pkgid) const
{
    return kContains(m_packages, [pkgid](const auto &x) {
        return x.archPkgIds.contains(pkgid) || x.nonarchPkgIds.contains(pkgid);
    });
}

void PackageKitResource::updateDetail(const QString &packageID,
                                      const QStringList & /*updates*/,
                                      const QStringList &obsoletes,
                                      const QStringList &vendorUrls,
                                      const QStringList & /*bugzillaUrls*/,
                                      const QStringList & /*cveUrls*/,
                                      PackageKit::Transaction::Restart restart,
                                      const QString &_updateText,
                                      const QString & /*changelog*/,
                                      PackageKit::Transaction::UpdateState state,
                                      const QDateTime & /*issued*/,
                                      const QDateTime & /*updated*/)
{
#if defined(WITH_MARKDOWN)
    const QByteArray xx = _updateText.toUtf8();
    MMIOT *markdownHandle = mkd_string(xx.constData(), _updateText.size(), {});

#ifdef MARKDOWN3
    mkd_flag_t *flags = mkd_flags();
    mkd_set_flag_num(flags, MKD_FENCEDCODE);
    mkd_set_flag_num(flags, MKD_GITHUBTAGS);
    mkd_set_flag_num(flags, MKD_AUTOLINK);
    if (!mkd_compile(markdownHandle, flags)) {
#else
    if (!mkd_compile(markdownHandle, MKD_FENCEDCODE | MKD_GITHUBTAGS | MKD_AUTOLINK)) {
#endif
        m_changelog = _updateText;
    } else {
        char *htmlDocument;
        const int size = mkd_document(markdownHandle, &htmlDocument);

        m_changelog = QString::fromUtf8(htmlDocument, size);
    }
    mkd_cleanup(markdownHandle);
#ifdef MARKDOWN3
    mkd_free_flags(flags);
#endif

#else
    m_changelog = _updateText;
#endif

    const auto name = PackageKit::Daemon::packageName(packageID);

    QString info;
    addIfNotEmpty(i18n("Obsoletes:"), joinPackages(obsoletes, {}, name), info);
    addIfNotEmpty(i18n("Release Notes:"), changelog(), info);
    addIfNotEmpty(i18n("Update State:"), PackageKitMessages::updateStateMessage(state), info);
    addIfNotEmpty(i18n("Restart:"), PackageKitMessages::restartMessage(restart), info);

    if (!vendorUrls.isEmpty()) {
        addIfNotEmpty(i18n("Vendor:"), urlToLinks(vendorUrls).join(QLatin1String(", ")), info);
    }

    Q_EMIT changelogFetched(info);
}

PackageKitBackend *PackageKitResource::backend() const
{
    return qobject_cast<PackageKitBackend *>(parent());
}

QString PackageKitResource::sizeDescription()
{
    auto baseDescription = AbstractResource::sizeDescription();

    if (state() != AbstractResource::State::None) {
        return baseDescription;
    }

    if (!m_dependencies.hasFetchedDependencies()) {
        fetchDetails();
        updatePackageIdForDependencies();
        return baseDescription;
    }

    const auto dependenciesCount = m_dependencies.dependencies().count();
    if (dependenciesCount == 0) {
        return baseDescription;
    }
    return i18np("%2 (plus %1 dependency)", "%2 (plus %1 dependencies)", dependenciesCount, baseDescription);
}

QString PackageKitResource::sourceIcon() const
{
    // Get repository name from package ID
    QString pkgid = availablePackageId().isEmpty() ? installedPackageId() : availablePackageId();
    if (!pkgid.isEmpty()) {
        QString repoName = PackageKit::Daemon::packageData(pkgid);
        // Clean up repo name
        if (repoName.startsWith(QStringLiteral("installed:"))) {
            repoName = repoName.mid(10);
        }

        // Return different icons based on repository
        if (repoName.startsWith(QStringLiteral("rpmfusion-"))) {
            return QStringLiteral("application-x-rpm");
        } else if (repoName.startsWith(QStringLiteral("@copr:")) || repoName.startsWith(QStringLiteral("copr:"))) {
            return QStringLiteral("cloud-upload");
        } else if (repoName == QStringLiteral("fedora") || repoName == QStringLiteral("updates") || repoName == QStringLiteral("updates-testing")
                   || repoName.isEmpty() || repoName == QStringLiteral("installed")) {
            return QStringLiteral("fedora-logo-icon");
        }
    }
    return QStringLiteral("package-x-generic");
}

QStringList PackageKitResource::topObjects() const
{
    return s_topObjects;
}

QStringList PackageKitResource::bottomObjects() const
{
    return s_bottomObjects;
}

void PackageKitResource::updatePackageIdForDependencies()
{
    const auto packageId = isInstalled() ? installedPackageId() : availablePackageId();
    m_dependencies.setPackageId(packageId);
    m_dependencies.refresh(); // In case packageId didn't actually change.
}

bool PackageKitResource::extendsItself() const
{
    const auto extendsResources = backend()->resourcesByPackageNames<QVector<AbstractResource *>>(extends());
    if (extendsResources.isEmpty()) {
        return false;
    }

    const auto ourPackageNames = allPackageNames();
    for (auto resource : extendsResources) {
        auto pkResource = qobject_cast<PackageKitResource *>(resource);
        if (pkResource->allPackageNames() != ourPackageNames) {
            return false;
        }
    }
    return true;
}

void PackageKitResource::runService(KService::Ptr service) const
{
    auto *job = new KIO::ApplicationLauncherJob(service);
    connect(job, &KJob::finished, this, [this, service](KJob *job) {
        if (job->error()) {
            Q_EMIT backend()->passiveMessage(i18n("Failed to start '%1': %2", service->name(), job->errorString()));
        }
    });

    job->start();
}

bool PackageKitResource::isCritical() const
{
    return false;
}

#include "moc_PackageKitResource.cpp"
