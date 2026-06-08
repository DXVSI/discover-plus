/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakSourcesBackend.h"
#include "FlatpakBackend.h"
#include "FlatpakResource.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QStandardPaths>
#include <QTemporaryFile>
#include <glib.h>
#include <resources/DiscoverAction.h>
#include <resources/StoredResultsStream.h>

using namespace Qt::StringLiterals;
using namespace Utils;

class FlatpakSourceItem : public QStandardItem
{
public:
    FlatpakSourceItem(const QString &text, FlatpakRemote *remote, FlatpakBackend *backend)
        : QStandardItem(text)
        , m_remote(remote)
        , m_backend(backend)
    {
        g_object_ref(remote);
    }
    ~FlatpakSourceItem()
    {
        g_object_unref(m_remote);
    }

    void setFlatpakInstallation(FlatpakInstallation *installation)
    {
        m_installation = installation;
    }

    FlatpakInstallation *flatpakInstallation() const
    {
        return m_installation;
    }

    void setData(const QVariant &value, int role) override
    {
        // We check isCheckable() so the initial setting of the item doesn't trigger a change
        if (role == Qt::CheckStateRole && isCheckable()) {
            const bool disabled = flatpak_remote_get_disabled(m_remote);
            const bool requestedDisabled = Qt::Unchecked == value;
            if (disabled != requestedDisabled) {
                flatpak_remote_set_disabled(m_remote, requestedDisabled);
                g_autoptr(GError) error = nullptr;
                if (!flatpak_installation_modify_remote(m_installation, m_remote, nullptr, &error)) {
                    qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "set disabled failed" << error->message;
                    flatpak_remote_set_disabled(m_remote, disabled);
                    return;
                }

                setRemoteLoaded(requestedDisabled);
            }
        }
        QStandardItem::setData(value, role);
    }

    void setRemoteLoaded(bool loaded)
    {
        if (loaded) {
            m_backend->loadRemote(m_installation, m_remote);
        } else {
            m_backend->unloadRemote(m_installation, m_remote);
        }
    }

    FlatpakRemote *remote() const
    {
        return m_remote;
    }

private:
    FlatpakInstallation *m_installation = nullptr;
    FlatpakRemote *const m_remote;
    FlatpakBackend *const m_backend;
};

FlatpakSourcesBackend::FlatpakSourcesBackend(const QVector<FlatpakInstallation *> &installations, AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , m_preferredInstallation(installations.constFirst())
    , m_sources(new QStandardItemModel(this))
    , m_flathubAction(new DiscoverAction(u"flatpak-discover"_s, i18n("Add Flathub"), this))
    , m_saveAction(new DiscoverAction(u"dialog-ok-apply"_s, i18n("Apply Changes"), this))
    , m_noSourcesItem(new QStandardItem(QStringLiteral("-")))
{
    m_saveAction->setVisible(false);
    m_saveAction->setToolTip(i18n("Changes to the priority of Flatpak sources must be applied before they will take effect."));
    connect(m_saveAction, &DiscoverAction::triggered, this, &FlatpakSourcesBackend::save);

    m_flathubAction->setObjectName(QStringLiteral("flathub"));
    m_flathubAction->setToolTip(i18n("Makes it possible to easily install the applications listed in https://flathub.org"));
    connect(m_flathubAction, &DiscoverAction::triggered, this, [this]() {
        addSource(QStringLiteral("https://dl.flathub.org/repo/flathub.flatpakrepo"));
    });

    m_noSourcesItem->setEnabled(false);
    if (m_sources->rowCount() == 0) {
        m_sources->appendRow(m_noSourcesItem);
    }
}

FlatpakSourcesBackend::~FlatpakSourcesBackend()
{
    QStringList ids;
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto it = m_sources->item(i);
        ids << it->data(IdRole).toString();
    }

    auto conf = KSharedConfig::openConfig();
    KConfigGroup group = conf->group(u"FlatpakSources"_s);
    group.writeEntry("Sources", ids);

    if (!m_noSourcesItem->model()) {
        delete m_noSourcesItem;
    }
}

void FlatpakSourcesBackend::save()
{
    int last = INT_MIN;
    for (int i = m_sources->rowCount() - 1; i >= 0; --i) {
        auto it = m_sources->item(i);
        const int prio = it->data(PrioRole).toInt();
        if (prio <= last) {
            FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem *>(it);
            flatpak_remote_set_prio(sourceItem->remote(), ++last);
            g_autoptr(GError) error = nullptr;
            if (!flatpak_installation_modify_remote(sourceItem->flatpakInstallation(), sourceItem->remote(), nullptr, &error)) {
                qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "failed setting priorities" << error->message;
            }

            it->setData(last, PrioRole);
        } else {
            last = prio;
        }
    }
    m_saveAction->setVisible(false);
}

QAbstractItemModel *FlatpakSourcesBackend::sources()
{
    return m_sources;
}

bool FlatpakSourcesBackend::addSource(const QString &id)
{
    auto backend = qobject_cast<FlatpakBackend *>(parent());
    const QUrl flatpakrepoUrl(id);

    if (id.isEmpty() || !flatpakrepoUrl.isValid()) {
        return false;
    }

    auto addSource = [=](const StreamResult &res) {
        if (res.resource) {
            backend->installApplication(res.resource);
        } else {
            Q_EMIT backend->passiveMessage(i18n("Could not add the source %1", flatpakrepoUrl.toDisplayString()));
        }
    };

    if (flatpakrepoUrl.isLocalFile()) {
        auto stream = new ResultsStream(QStringLiteral("FlatpakSource-") + flatpakrepoUrl.toDisplayString());
        backend->addSourceFromFlatpakRepo(flatpakrepoUrl, stream);
        connect(stream, &ResultsStream::resourcesFound, this, [addSource](const QVector<StreamResult> &res) {
            addSource(res.constFirst());
        });
    } else {
        AbstractResourcesBackend::Filters filter;
        filter.resourceUrl = flatpakrepoUrl;
        auto stream = new StoredResultsStream({backend->search(filter)});
        connect(stream, &StoredResultsStream::finished, this, [addSource, stream]() {
            const auto res = stream->resources();
            addSource(res.value(0));
        });
    }
    return true;
}

QStandardItem *FlatpakSourcesBackend::sourceByDisambiguatedId(const QString &id) const
{
    QStandardItem *sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto it = m_sources->item(i);
        if (it->data(DisambiguatedIdRole) == id) {
            sourceIt = it;
            break;
        }
    }
    return sourceIt;
}

QStandardItem *FlatpakSourcesBackend::sourceByUrl(const QString &_url) const
{
    QUrl url(_url);

    QStandardItem *sourceIt = nullptr;
    for (int i = 0, c = m_sources->rowCount(); i < c && !sourceIt; ++i) {
        auto it = m_sources->item(i);
        if (url.matches(it->data(Qt::StatusTipRole).toUrl(), QUrl::StripTrailingSlash)) {
            sourceIt = it;
            break;
        }
    }
    return sourceIt;
}

bool FlatpakSourcesBackend::removeSource(const QString &disambiguatedSourceId)
{
    auto sourceIt = sourceByDisambiguatedId(disambiguatedSourceId);
    if (sourceIt) {
        FlatpakSourceItem *sourceItem = static_cast<FlatpakSourceItem *>(sourceIt);
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) error = nullptr;
        const auto installation = sourceItem->flatpakInstallation();

        auto id = disambiguatedSourceId.left(disambiguatedSourceId.lastIndexOf(u'-'));

        g_autoptr(GPtrArray) refs = flatpak_installation_list_remote_refs_sync(installation, id.toUtf8().constData(), cancellable, &error);
        if (refs) {
            QHash<QString, QStringList> toRemoveHash;
            toRemoveHash.reserve(refs->len);
            QStringList toRemoveRefs;
            toRemoveRefs.reserve(refs->len);
            FlatpakBackend *backend = qobject_cast<FlatpakBackend *>(parent());
            for (uint i = 0; i < refs->len; i++) {
                FlatpakRef *ref = FLATPAK_REF(g_ptr_array_index(refs, i));

                g_autoptr(GError) error = nullptr;
                FlatpakInstalledRef *installedRef = flatpak_installation_get_installed_ref(installation,
                                                                                           flatpak_ref_get_kind(ref),
                                                                                           flatpak_ref_get_name(ref),
                                                                                           flatpak_ref_get_arch(ref),
                                                                                           flatpak_ref_get_branch(ref),
                                                                                           cancellable,
                                                                                           &error);
                if (installedRef) {
                    auto res = backend->getAppForInstalledRef(installation, installedRef);
                    const auto name = QString::fromUtf8(flatpak_ref_get_name(ref));
                    const auto refString = QString::fromUtf8(flatpak_ref_format_ref_cached(ref));
                    if (!name.endsWith(QLatin1String(".Locale"))) {
                        if (res) {
                            toRemoveHash[res->name()] << refString;
                        } else {
                            toRemoveHash[refString] << refString;
                        }
                    }
                    toRemoveRefs << refString;
                }
            }
            QStringList toRemove;
            toRemove.reserve(toRemoveHash.count());
            for (auto it = toRemoveHash.constBegin(), itEnd = toRemoveHash.constEnd(); it != itEnd; ++it) {
                if (it.value().count() > 1) {
                    toRemove << QStringLiteral("%1 - %2").arg(it.key(), it.value().join(QLatin1String(", ")));
                } else {
                    toRemove << it.key();
                }
            }
            toRemove.sort();

            if (!toRemove.isEmpty()) {
                m_proceedFunctions.push([this, toRemoveRefs, installation, id] {
                    g_autoptr(GError) localError = nullptr;
                    g_autoptr(GCancellable) cancellable = g_cancellable_new();
                    g_autoptr(FlatpakTransaction) transaction = flatpak_transaction_new_for_installation(installation, cancellable, &localError);
                    for (const QString &instRef : std::as_const(toRemoveRefs)) {
                        const QByteArray refString = instRef.toUtf8();
                        flatpak_transaction_add_uninstall(transaction, refString.constData(), &localError);
                        if (localError) {
                            return;
                        }
                    }

                    if (flatpak_transaction_run(transaction, cancellable, &localError)) {
                        removeSource(id);
                    }
                });

                Q_EMIT proceedRequest(i18n("Removing '%1'", id),
                                      i18n("To remove this repository, the following applications must be uninstalled:<ul><li>%1</li></ul>",
                                           toRemove.join(QStringLiteral("</li><li>"))));
                return false;
            }
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "could not list refs in repo" << id << error->message;
        }

        g_autoptr(GError) errorRemoveRemote = nullptr;
        if (flatpak_installation_remove_remote(installation, id.toUtf8().constData(), cancellable, &errorRemoveRemote)) {
            sourceItem->setRemoteLoaded(false);
            m_sources->removeRow(sourceItem->row());

            if (m_sources->rowCount() == 0) {
                m_sources->appendRow(m_noSourcesItem);
            }
            return true;
        } else {
            Q_EMIT passiveMessage(i18n("Failed to remove %1 remote repository: %2", id, QString::fromUtf8(errorRemoveRemote->message)));
            return false;
        }
    } else {
        Q_EMIT passiveMessage(i18n("Could not find %1", disambiguatedSourceId));
        return false;
    }

    return false;
}

QVariantList FlatpakSourcesBackend::actions() const
{
    return {QVariant::fromValue<QObject *>(m_flathubAction)};
}

void FlatpakSourcesBackend::addRemote(FlatpakRemote *remote, FlatpakInstallation *installation)
{
    if (flatpak_remote_get_noenumerate(remote)) {
        return;
    }
    const QString id = QString::fromUtf8(flatpak_remote_get_name(remote));
    const QString title = copyAndFree(flatpak_remote_get_title(remote));
    const QString disambiguatedId =
        QString::fromUtf8(flatpak_remote_get_name(remote)) + (flatpak_installation_get_is_user(installation) ? u"-user"_s : u"-system"_s);
    const QUrl remoteUrl(copyAndFree(flatpak_remote_get_url(remote)));

    const auto theActions = actions();
    for (const auto &variant : theActions) {
        auto action = variant.value<DiscoverAction *>();
        if (action && action->objectName() == id) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    QString label = !title.isEmpty() ? title : id;
    if (flatpak_installation_get_is_user(installation)) {
        label = i18n("%1 (user)", label);
    }

    for (int i = 0, c = m_sources->rowCount(); i < c; ++i) {
        auto genItem = m_sources->item(i);
        if (genItem == m_noSourcesItem) {
            continue;
        }

        auto item = static_cast<FlatpakSourceItem *>(m_sources->item(i));
        if (item->data(Qt::StatusTipRole) == remoteUrl && item->flatpakInstallation() == installation) {
            qCDebug(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "we already have an item for this" << remoteUrl;
            return;
        }
    }

    auto backend = qobject_cast<FlatpakBackend *>(parent());
    auto it = new FlatpakSourceItem(label, remote, backend);
    const int prio = flatpak_remote_get_prio(remote);
    it->setData(remoteUrl.isLocalFile() ? remoteUrl.toLocalFile() : remoteUrl.host(), Qt::ToolTipRole);
    it->setData(remoteUrl, Qt::StatusTipRole);
    it->setData(id, IdRole);
    it->setData(disambiguatedId, DisambiguatedIdRole);
    it->setData(prio, PrioRole);
    it->setCheckState(flatpak_remote_get_disabled(remote) ? Qt::Unchecked : Qt::Checked);
#if FLATPAK_CHECK_VERSION(1, 4, 0)
    it->setData(copyAndFree(flatpak_remote_get_icon(remote)), IconUrlRole);
#endif
    it->setCheckable(true);
    it->setFlatpakInstallation(installation);

    // Add the remotes before those with lower priorities, after the rest.
    // We disambiguate with internal discover settings
    const auto conf = KSharedConfig::openConfig();
    const KConfigGroup group = conf->group(u"FlatpakSources"_s);
    const auto ids = group.readEntry<QStringList>("Sources", QStringList());
    const int ourIdx = ids.indexOf(id);

    int idx, c;
    for (c = m_sources->rowCount(), idx = 0; idx < c; ++idx) {
        const auto compIt = m_sources->item(idx);
        if (prio > compIt->data(PrioRole).toInt()) {
            break;
        }
        const int compIdx = ids.indexOf(compIt->data(IdRole).toString());
        if (compIdx >= ourIdx) {
            break;
        }
    }

    m_sources->insertRow(idx, it);
    if (m_sources->rowCount() == 1) {
        Q_EMIT firstDisambiguatedSourceIdChanged();
    }
    Q_EMIT lastDisambiguatedSourceIdChanged();

    if (m_sources->rowCount() > 0) {
        m_sources->takeRow(m_noSourcesItem->row());
    }
}

QString FlatpakSourcesBackend::idDescription()
{
    return i18n("Enter a Flatpak repository URI (*.flatpakrepo):");
}

bool FlatpakSourcesBackend::moveSource(const QString &sourceId, int delta)
{
    auto item = sourceByDisambiguatedId(sourceId);
    if (!item) {
        return false;
    }
    const auto row = item->row();
    auto prevRow = m_sources->takeRow(row);
    Q_ASSERT(!prevRow.isEmpty());

    const auto destRow = row + delta;
    m_sources->insertRow(destRow, prevRow);
    if (destRow == 0 || row == 0) {
        Q_EMIT firstDisambiguatedSourceIdChanged();
    }
    if (destRow == m_sources->rowCount() - 1 || row == m_sources->rowCount() - 1) {
        Q_EMIT lastDisambiguatedSourceIdChanged();
    }
    m_saveAction->setVisible(true);
    return true;
}

int FlatpakSourcesBackend::originIndex(const QString &disambiguatedSourceId) const
{
    auto item = sourceByDisambiguatedId(disambiguatedSourceId);
    return item ? item->row() : INT_MAX;
}

void FlatpakSourcesBackend::cancel()
{
    m_proceedFunctions.pop();
}

void FlatpakSourcesBackend::proceed()
{
    m_proceedFunctions.pop()();
}

#include "moc_FlatpakSourcesBackend.cpp"
