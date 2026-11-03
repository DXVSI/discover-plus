/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// Qt includes
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

// KDE includes
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNSCore/Provider>
#include <KNSCore/Question>
#include <KNSCore/QuestionManager>
#include <KNSCore/ResultsStream>
#include <KWindowSystem>

// DiscoverCommon includes
#include "Category/Category.h"
#include "Transaction/Transaction.h"
#include "Transaction/TransactionModel.h"

// Own includes
#include "KNSBackend.h"
#include "KNSResource.h"
#include "KNSReviews.h"
#include "KNSTransaction.h"
#include "utils.h"
#include <resources/StandardBackendUpdater.h>

using namespace Qt::StringLiterals;

static const int ENGINE_PAGE_SIZE = 100;

class KNSBackendFactory : public AbstractResourcesBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DISCOVER_PLUGIN_IID)
    Q_INTERFACES(AbstractResourcesBackendFactory)
public:
    KNSBackendFactory()
    {
        connect(KNSCore::QuestionManager::instance(), &KNSCore::QuestionManager::askQuestion, this, [](KNSCore::Question *question) {
            const auto transactions = TransactionModel::global()->transactions();
            for (auto t : transactions) {
                const auto transaction = dynamic_cast<KNSTransaction *>(t);
                if (!transaction) {
                    continue;
                }

                if (question->entry().uniqueId() == transaction->uniqueId()) {
                    switch (question->questionType()) {
                    case KNSCore::Question::ContinueCancelQuestion:
                        transaction->addQuestion(question);
                        return;
                    default:
                        Q_EMIT transaction->passiveMessage(i18n("Unsupported question:\n%1", question->question()));
                        question->setResponse(KNSCore::Question::InvalidResponse);
                        transaction->setStatus(Transaction::CancelledStatus);
                        break;
                    }
                    return;
                }
            }
            qWarning() << "Question for unknown transaction" << question->question() << question->questionType();
            question->setResponse(KNSCore::Question::InvalidResponse);
        });
    }

    QVector<AbstractResourcesBackend *> newInstance(QObject *parent, const QString & /*name*/) const override
    {
        QVector<AbstractResourcesBackend *> ret;
        const QStringList availableConfigFiles = KNSCore::EngineBase::availableConfigFiles();
        for (const QString &configFile : availableConfigFiles) {
            auto bk = new KNSBackend(parent, QStringLiteral("plasma"), configFile);
            if (bk->isValid())
                ret += bk;
            else
                delete bk;
        }
        return ret;
    }
};

class KNSResultsStream : public ResultsStream
{
    Q_OBJECT
public:
    KNSResultsStream(KNSBackend *backend, const QString &objectName)
        : ResultsStream(objectName)
        , m_backend(backend)
    {
        // Abort streams when the backend gets invalidated
        connect(m_backend, &KNSBackend::invalidated, this, &KNSResultsStream::finish);

        if (!m_backend->isValid()) {
            qWarning() << "Erroneously starting a kns stream with an invalid backend" << m_backend->name();
        }
    }

    void setRequest(const KNSCore::SearchRequest &request)
    {
        Q_ASSERT(!m_started);
        m_started = true;
        KNSCore::ResultsStream *job = m_backend->engine()->search(request);
        connect(job, &KNSCore::ResultsStream::entriesFound, this, &KNSResultsStream::addEntries);
        connect(job, &KNSCore::ResultsStream::finished, this, &KNSResultsStream::finish);
        connect(this, &ResultsStream::fetchMore, job, &KNSCore::ResultsStream::fetchMore);
        job->fetch();
    }

    void addEntries(const KNSCore::Entry::List &entries)
    {
        // Should probably address that KNSCore::ResultsStream would return the Entry several times...
        const auto selectedEntries = kFilter<KNSCore::Entry::List>(entries, [this](const auto &entry) {
            return !m_sent.contains(entry.uniqueId());
        });
        const auto res = kTransform<QList<StreamResult>>(selectedEntries, [this](const auto &entry) {
            return StreamResult{m_backend->resourceForEntry(entry), 0};
        });
        for (const auto &entry : selectedEntries) {
            m_sent.insert(entry.uniqueId());
        }
        Q_EMIT resourcesFound(res);
    }

    bool hasStarted() const
    {
        return m_started;
    }

private:
    QSet<QString> m_sent;
    KNSBackend *const m_backend;
    bool m_started = false;
};

KNSBackend::KNSBackend(QObject *parent, const QString &iconName, const QString &knsrc)
    : AbstractResourcesBackend(parent)
    , m_fetching(false)
    , m_isValid(true)
    , m_reviews(new KNSReviews(this))
    , m_name(knsrc)
    , m_iconName(iconName)
    , m_updater(new StandardBackendUpdater(this))
{
    const QString fileName = QFileInfo(m_name).fileName();
    setName(fileName);
    setObjectName(knsrc);

    static const QSet<QStringView> knsrcPlasma = []() {
        QSet<QStringView> ret = {
            u"aurorae.knsrc",       u"icons.knsrc",           u"kfontinst.knsrc", u"lookandfeel.knsrc",
            u"plasma-themes.knsrc", u"plasmoids.knsrc",       u"wallpaper.knsrc", u"wallpaper-mobile.knsrc",
            u"xcursor.knsrc",       u"cgcgtk3.knsrc",         u"cgcicon.knsrc",   u"cgctheme.knsrc",
            u"comic.knsrc",         u"colorschemes.knsrc",    u"emoticons.knsrc", u"plymouth.knsrc",
            u"sddmtheme.knsrc",     u"wallpaperplugin.knsrc", u"ksplash.knsrc",
        };

        if (KWindowSystem::isPlatformX11()) {
            ret.insert(u"kwinswitcher-x11.knsrc");
            ret.insert(u"kwineffect-x11.knsrc");
            ret.insert(u"kwinscripts-x11.knsrc");
            ret.insert(u"window-decorations-x11.knsrc");
        } else {
            ret.insert(u"kwinswitcher.knsrc");
            ret.insert(u"kwineffect.knsrc");
            ret.insert(u"kwinscripts.knsrc");
            ret.insert(u"window-decorations.knsrc");
        }

        return ret;
    }();

    const bool isPlasmaCategory = knsrcPlasma.contains(fileName);
    static const bool weAreOnPlasma = qgetenv("XDG_CURRENT_DESKTOP") == "KDE";
    if (isPlasmaCategory && !weAreOnPlasma) {
        markInvalid(QStringLiteral("Skipping Plasma categories outside of Plasma."));
        return;
    }

    const KConfig conf(m_name, KConfig::SimpleConfig);
    const bool hasVersionlessGrp = conf.hasGroup(u"KNewStuff"_s);
    if (!conf.hasGroup(u"KNewStuff3"_s) && !hasVersionlessGrp) {
        markInvalid(QStringLiteral("Config group not found! Check your KNSCore installation."));
        return;
    }

    m_categories = QStringList{fileName};

    const KConfigGroup group = hasVersionlessGrp ? conf.group(u"KNewStuff"_s) : conf.group(u"KNewStuff3"_s);
    m_extends = group.readEntry("Extends", QStringList());

    setFetching(true);

    // This ensures we have something to track when checking after the initialization timeout
    connect(this, &KNSBackend::initialized, this, [this]() {
        m_initialized = true;
    });
    // If we have not initialized in 60 seconds, consider this KNS backend invalid
    QTimer::singleShot(60000, this, [this]() {
        if (!m_initialized) {
            markInvalid(i18n("Backend %1 took too long to initialize", m_displayName));
        }
    });

    const CategoryFilter filter = {CategoryFilter::CategoryNameFilter, fileName};
    const QSet<QString> backendName = {name()};
    m_displayName = group.readEntry("Name", QString());
    if (m_displayName.isEmpty()) {
        m_displayName = fileName.mid(0, fileName.indexOf(QLatin1Char('.')));
        m_displayName[0] = m_displayName[0].toUpper();
    }
    m_hasApplications = group.readEntry<bool>("X-Discover-HasApplications", false);

    const QStringList cats = group.readEntry<QStringList>("Categories", QStringList{});
    QList<std::shared_ptr<Category>> categories;
    if (cats.count() > 1) {
        m_categories += cats;
        for (const auto &cat : cats) {
            if (m_hasApplications)
                categories << std::make_shared<Category>(cat,
                                                         QStringLiteral("applications-other"),
                                                         CategoryFilter{CategoryFilter::CategoryNameFilter, cat},
                                                         backendName,
                                                         QList<std::shared_ptr<Category>>{},
                                                         Category::Type::Addon);
            else
                categories << std::make_shared<Category>(cat,
                                                         QStringLiteral("plasma"),
                                                         CategoryFilter{CategoryFilter::CategoryNameFilter, cat},
                                                         backendName,
                                                         QList<std::shared_ptr<Category>>{},
                                                         Category::Type::Addon);
        }
    }

    QList<std::shared_ptr<Category>> topCategories{categories};
    for (const auto &cat : std::as_const(categories)) {
        const QString catName = cat->name().append(QLatin1Char('/'));
        for (const auto &potentialSubCat : std::as_const(categories)) {
            if (potentialSubCat->name().startsWith(catName)) {
                cat->addSubcategory(potentialSubCat);
                topCategories.removeOne(potentialSubCat);
            }
        }
    }

    m_engine = new KNSCore::EngineBase(this);
    connect(m_engine, &KNSCore::EngineBase::signalErrorCode, this, &KNSBackend::slotErrorCode);
    connect(m_engine, &KNSCore::EngineBase::providerAdded, this, [this] {
        setFetching(false);
    });

    connect(m_engine, &KNSCore::EngineBase::signalCategoriesMetadataLoaded, this, [categories](const QList<KNSCore::CategoryMetadata> &categoryMetadatas) {
        for (const KNSCore::CategoryMetadata &category : categoryMetadatas) {
            for (const std::shared_ptr<Category> &cat : std::as_const(categories)) {
                if (cat->matchesCategoryName(category.name())) {
                    cat->setName(category.displayName());
                    break;
                }
            }
        }
    });
    m_engine->init(m_name);

    if (m_hasApplications) {
        auto actualCategory = std::make_shared<Category>(m_displayName, QStringLiteral("applications-other"), filter, backendName, topCategories);
        std::shared_ptr<Category> applicationCategory = std::make_shared<Category>(i18n("Applications"), //
                                                                                   QStringLiteral("applications-internet"),
                                                                                   filter,
                                                                                   backendName,
                                                                                   QList<std::shared_ptr<Category>>{actualCategory});
        const QList<CategoryFilter> filters = {{CategoryFilter::CategoryNameFilter, QLatin1String("Application")}, filter};
        applicationCategory->setFilter({CategoryFilter::AndFilter, filters});
        m_categories.append(applicationCategory->name());
        m_rootCategories = {applicationCategory};
        // Make sure we filter out any apps which won't run on the current system architecture
        QStringList tagFilter = m_engine->tagFilter();
        if (QSysInfo::currentCpuArchitecture() == QLatin1String("arm")) {
            tagFilter << QLatin1String("application##architecture==armhf");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("arm64")) {
            tagFilter << QLatin1String("application##architecture==arm64");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("i386")) {
            tagFilter << QLatin1String("application##architecture==x86");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("ia64")) {
            tagFilter << QLatin1String("application##architecture==x86-64");
        } else if (QSysInfo::currentCpuArchitecture() == QLatin1String("x86_64")) {
            tagFilter << QLatin1String("application##architecture==x86");
            tagFilter << QLatin1String("application##architecture==x86-64");
        }
        m_engine->setTagFilter(tagFilter);
    } else {
        const auto iconName = isPlasmaCategory ? QStringLiteral("plasma") : QStringLiteral("applications-other");
        auto actualCategory = std::make_shared<Category>(m_displayName, iconName, filter, backendName, categories, Category::Type::Addon);

        const auto topLevelName = isPlasmaCategory ? i18n("Plasma Addons") : i18n("Application Addons");
        auto addonsCategory =
            std::make_shared<Category>(topLevelName, iconName, filter, backendName, QList<std::shared_ptr<Category>>{actualCategory}, Category::Type::Addon);
        m_rootCategories = {addonsCategory};
    }

    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &KNSBackend::updatesCountChanged);
}

KNSBackend::~KNSBackend()
{
}

void KNSBackend::markInvalid(const QString &message)
{
    m_rootCategories.clear();
    qWarning() << "invalid kns backend!" << m_name << "because:" << message;
    m_isValid = false;
    setFetching(false);
    Q_EMIT initialized();
    Q_EMIT invalidated();
}

void KNSBackend::checkForUpdates()
{
    AbstractResourcesBackend::Filters filter;
    filter.state = AbstractResource::Upgradeable;
    search(filter);
}

void KNSBackend::setFetching(bool f)
{
    if (m_fetching != f) {
        m_fetching = f;
        if (!m_fetching) {
            Q_EMIT initialized();
            Q_EMIT contentsChanged();
        }
    }
}

bool KNSBackend::isValid() const
{
    return m_isValid;
}

KNSResource *KNSBackend::resourceForEntry(const KNSCore::Entry &entry)
{
    KNSResource *r = static_cast<KNSResource *>(m_resourcesByName.value(entry.uniqueId()));
    if (!r) {
        QStringList categories{name()};
        if (!m_rootCategories.isEmpty()) {
            categories << m_rootCategories.first()->name();
        }
        const auto cats = m_engine->categoriesMetadata2();
        const int catIndex = kIndexOf(cats, [&entry](const KNSCore::CategoryMetadata &cat) {
            return entry.category() == cat.id();
        });
        if (catIndex > -1) {
            categories << cats.at(catIndex).name();
        }
        if (m_hasApplications) {
            categories << QLatin1String("Application");
        }
        r = new KNSResource(entry, categories, this);
        m_resourcesByName.insert(entry.uniqueId(), r);
    } else {
        r->setEntry(entry);
    }
    return r;
}

void KNSBackend::statusChanged(const KNSCore::Entry &entry)
{
    resourceForEntry(entry);
}

void KNSBackend::slotErrorCode(const KNSCore::ErrorCode::ErrorCode &errorCode, const QString &message, const QVariant &metadata)
{
    QString error = message;
    qWarning() << "KNS error in" << m_displayName << ":" << errorCode << message << metadata;
    bool invalidFile = false;
    switch (errorCode) {
    case KNSCore::ErrorCode::UnknownError:
        // This is not supposed to be hit, of course, but any error coming to this point should be non-critical and safely ignored.
        break;
    case KNSCore::ErrorCode::NetworkError:
        // If we have a network error, we need to tell the user about it. This is almost always fatal, so mark invalid and tell the user.
        error = i18n("Network error in backend %1: %2", m_displayName, metadata.toInt());
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::OcsError:
        if (metadata.toInt() == 200) {
            // Too many requests, try again in a couple of minutes - perhaps we can simply postpone it automatically, and give a message?
            error = i18n("Too many requests sent to the server for backend %1. Please try again in a few minutes.", m_displayName);
        } else {
            // Unknown API error, usually something critical, mark as invalid and cry a lot
            error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
            markInvalid(error);
            invalidFile = true;
        }
        break;
    case KNSCore::ErrorCode::ConfigFileError:
        error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::ProviderError:
        error = i18n("Invalid %1 backend, contact your distributor.", m_displayName);
        markInvalid(error);
        invalidFile = true;
        break;
    case KNSCore::ErrorCode::InstallationError: {
        KNSResource *r = static_cast<KNSResource *>(m_resourcesByName.value(metadata.toString()));
        if (r) {
            // If the following is true, then we can safely assume that the entry was
            // attempted updated, but the update was aborted.
            // Specifically, we can also likely expect that the update failed because
            // KNSCore::Engine was unable to deduce which payload to use (which will
            // happen when an entry has more than one payload, and none of those match
            // the name of the originally downloaded file).
            // We cannot complete this in Discover (as we've no way to forward that
            // query to the user) but we can give them an idea of how to deal with the
            // situation some other way.
            // TODO: Once Discover has a way to forward queries to the user from transactions, this likely will no longer be needed
            if (r->entry().status() == KNSCore::Entry::Updateable) {
                error = i18n(
                    "Unable to complete the update of %1. You can try and perform this action through the Get Hot New Stuff dialog, which grants tighter "
                    "control. The reported error was:\n%2",
                    r->name(),
                    message);
            }
        }
        break;
    }
    case KNSCore::ErrorCode::ImageError:
        // Image fetching errors are not critical as such, but may lead to weird layout issues, might want handling...
        error = i18n("Could not fetch screenshot for the entry %1 in backend %2", metadata.toList().at(0).toString(), m_displayName);
        break;
    default:
        // Having handled all current error values, we should by all rights never arrive here, but for good order and future safety...
        error = i18n("Unhandled error in %1 backend. Contact your distributor.", m_displayName);
        break;
    }
    qWarning() << "kns error" << objectName() << error;
    if (!invalidFile)
        Q_EMIT passiveMessage(i18n("%1: %2", name(), error));
}

void KNSBackend::slotEntryEvent(const KNSCore::Entry &entry, KNSCore::Entry::EntryEvent event)
{
    switch (event) {
    case KNSCore::Entry::StatusChangedEvent:
        statusChanged(entry);
        break;
    case KNSCore::Entry::DetailsLoadedEvent:
        detailsLoaded(entry);
        break;
    case KNSCore::Entry::AdoptedEvent:
    case KNSCore::Entry::UnknownEvent:
    default:
        break;
    }
}

Transaction *KNSBackend::removeApplication(AbstractResource *app)
{
    auto res = qobject_cast<KNSResource *>(app);
    return new KNSTransaction(res, Transaction::RemoveRole);
}

Transaction *KNSBackend::installApplication(AbstractResource *app)
{
    auto res = qobject_cast<KNSResource *>(app);
    return new KNSTransaction(res, Transaction::InstallRole);
}

Transaction *KNSBackend::installApplication(AbstractResource *app, const AddonList & /*addons*/)
{
    return installApplication(app);
}

int KNSBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

AbstractReviewsBackend *KNSBackend::reviewsBackend() const
{
    return m_reviews;
}

static ResultsStream *voidStream()
{
    return new ResultsStream(QStringLiteral("KNS-void"), {});
}

template<typename T>
void KNSBackend::deferredResultStream(KNSResultsStream *stream, T start)
{
    if (m_fetching) {
        auto startOnce = [stream, start] {
            if (stream->hasStarted()) {
                return;
            }
            start();
        };

        // If it's not ready to take queries, wait a bit longer
        connect(this, &KNSBackend::initialized, stream, startOnce, Qt::QueuedConnection);
    } else {
        QTimer::singleShot(0, stream, start);
    }
}

ResultsStream *KNSBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    if (!m_isValid || (!filter.resourceUrl.isEmpty() && filter.resourceUrl.scheme() != QLatin1String("kns")) || !filter.mimetype.isEmpty())
        return voidStream();

    if (filter.resourceUrl.scheme() == QLatin1String("kns")) {
        return findResourceByPackageName(filter.resourceUrl);
    } else if (filter.state >= AbstractResource::Installed) {
        auto stream = new KNSResultsStream(this, "KNS-installed-"_L1 + name());
        const auto start = [this, stream, filter]() {
            if (m_isValid) {
                const auto knsFilter = filter.state == AbstractResource::Installed ? KNSCore::Filter::Installed : KNSCore::Filter::Updates;
                stream->setRequest(KNSCore::SearchRequest(KNSCore::SortMode::Newest, knsFilter, {}, {}, -1, ENGINE_PAGE_SIZE));
            }
        };
        deferredResultStream(stream, start);
        return stream;
        // Make sure we actually are one of the requested categories before searching
    } else if ((filter.category && std::ranges::any_of(m_categories, [&filter](const QString &cat) {
                    return filter.category->matchesCategoryName(cat);
                }))) {
        return searchStream(filter.search);
    }
    return voidStream();
}

KNSResultsStream *KNSBackend::searchStream(const QString &searchText)
{
    auto stream = new KNSResultsStream(this, QLatin1String("KNS-search-") + name());
    auto start = [this, stream, searchText]() {
        Q_ASSERT(!m_fetching);
        if (!m_isValid) {
            qWarning() << "querying an invalid backend";
            stream->finish();
            return;
        }
        KNSCore::SearchRequest p(KNSCore::SortMode::Newest, KNSCore::Filter::None, searchText, {}, 0, ENGINE_PAGE_SIZE);
        stream->setRequest(p);
    };
    deferredResultStream(stream, start);
    return stream;
}

ResultsStream *KNSBackend::findResourceByPackageName(const QUrl &search)
{
    if (search.scheme() != QLatin1String("kns") || search.host() != name())
        return voidStream();

    const auto pathParts = search.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.size() != 1) {
        Q_EMIT passiveMessage(i18n("Wrong KNewStuff URI: %1", search.toString()));
        return voidStream();
    }
    const auto entryid = pathParts.at(0);

    auto stream = new KNSResultsStream(this, QLatin1String("KNS-byname-") + entryid);
    auto start = [entryid, stream]() {
        KNSCore::SearchRequest query(KNSCore::SortMode::Newest, KNSCore::Filter::ExactEntryId, entryid, {}, 0, ENGINE_PAGE_SIZE);
        stream->setRequest(query);
    };
    deferredResultStream(stream, start);
    return stream;
}

AbstractBackendUpdater *KNSBackend::backendUpdater() const
{
    return m_updater;
}

QString KNSBackend::displayName() const
{
    return QStringLiteral("KNewStuff");
}

void KNSBackend::detailsLoaded(const KNSCore::Entry &entry)
{
    auto res = resourceForEntry(entry);
    Q_EMIT res->longDescriptionChanged();
}

#include "KNSBackend.moc"
#include "moc_KNSBackend.cpp"
