/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018 Abhijeet Sharma <sharma.abhijeet2096@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FwupdSourcesBackend.h"

#include <KLocalizedString>
#include <QString>

class FwupdSourcesModel : public QStandardItemModel
{
    Q_OBJECT
public:
    FwupdSourcesModel(FwupdSourcesBackend *backend)
        : QStandardItemModel(backend)
        , m_backend(backend)
    {
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        auto item = itemFromIndex(index);
        if (!item)
            return false;

        FwupdRemote *remote = fwupd_client_get_remote_by_id(m_backend->backend->m_client.get(),
                                                            item->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),
                                                            nullptr,
                                                            nullptr);
        switch (role) {
        case Qt::CheckStateRole: {
            if (value == Qt::Checked) {
                m_backend->m_currentItem = item;
                if (fwupd_remote_has_flag(remote, FWUPD_REMOTE_FLAG_APPROVAL_REQUIRED)) {
                    QString eulaText = i18n("The remote %1 require that you accept their license:\n %2",
                                            QString::fromUtf8(fwupd_remote_get_title(remote)),
                                            QString::fromUtf8(fwupd_remote_get_agreement(remote)));
                    Q_EMIT m_backend->proceedRequest(i18n("Review EULA"), eulaText);
                } else {
                    m_backend->proceed();
                }
            } else if (value.toInt() == Qt::Unchecked) {
                g_autoptr(GError) error = nullptr;
                if (fwupd_client_modify_remote(m_backend->backend->m_client.get(), fwupd_remote_get_id(remote), "Enabled", "false", nullptr, &error))
                    item->setCheckState(Qt::Unchecked);
                else
                    qWarning() << "could not disable remote" << remote << error->message;
            }
            return true;
        }
        }
        return false;
    }

private:
    FwupdSourcesBackend *const m_backend;
};

FwupdSourcesBackend::FwupdSourcesBackend(AbstractResourcesBackend *parent)
    : AbstractSourcesBackend(parent)
    , backend(qobject_cast<FwupdBackend *>(parent))
    , m_sources(new FwupdSourcesModel(this))
{
    populateSources();
}

void FwupdSourcesBackend::populateSources()
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) remotes = fwupd_client_get_remotes(backend->m_client.get(), nullptr, &error);
    if (!remotes) {
        qWarning() << "could not list fwupd remotes" << error->message;
        return;
    }

    for (uint i = 0; i < remotes->len; i++) {
        FwupdRemote *remote = (FwupdRemote *)g_ptr_array_index(remotes, i);
        if (fwupd_remote_get_kind(remote) == FWUPD_REMOTE_KIND_LOCAL)
            continue;
        const QString id = QString::fromUtf8(fwupd_remote_get_id(remote));
        if (id.isEmpty())
            continue;

        QStandardItem *it = new QStandardItem(id);
        it->setData(id, AbstractSourcesBackend::IdRole);
        it->setData(QVariant(QString::fromUtf8(fwupd_remote_get_title(remote))), Qt::ToolTipRole);
        it->setCheckable(true);
        it->setCheckState(fwupd_remote_has_flag(remote, FWUPD_REMOTE_FLAG_ENABLED) ? Qt::Checked : Qt::Unchecked);
        m_sources->appendRow(it);
    }
}

QAbstractItemModel *FwupdSourcesBackend::sources()
{
    return m_sources;
}

bool FwupdSourcesBackend::addSource(const QString &id)
{
    qWarning() << "Fwupd Error: Custom Addition of Sources Not Allowed"
               << "Remote-ID" << id;
    return false;
}

bool FwupdSourcesBackend::removeSource(const QString &id)
{
    qWarning() << "Fwupd Error: Removal of Sources Not Allowed"
               << "Remote-ID" << id;
    return false;
}

QVariantList FwupdSourcesBackend::actions() const
{
    return {};
}

void FwupdSourcesBackend::cancel()
{
    FwupdRemote *remote = fwupd_client_get_remote_by_id(backend->m_client.get(),
                                                        m_currentItem->data(AbstractSourcesBackend::IdRole).toString().toUtf8().constData(),
                                                        nullptr,
                                                        nullptr);
    m_currentItem->setCheckState(fwupd_remote_has_flag(remote, FWUPD_REMOTE_FLAG_ENABLED) ? Qt::Checked : Qt::Unchecked);

    m_currentItem = nullptr;
}

void FwupdSourcesBackend::proceed()
{
    const QString id = m_currentItem->data(AbstractSourcesBackend::IdRole).toString();
    FwupdRemote *remote = fwupd_client_get_remote_by_id(backend->m_client.get(), id.toUtf8().constData(), nullptr, nullptr);
    g_autoptr(GError) error = nullptr;
    if (fwupd_client_modify_remote(backend->m_client.get(), fwupd_remote_get_id(remote), "Enabled", "true", nullptr, &error))
        m_currentItem->setData(Qt::Checked, Qt::CheckStateRole);
    else
        Q_EMIT passiveMessage(i18n("Could not enable remote %1: %2", id, QString::fromUtf8(error ? error->message : "")));

    m_currentItem = nullptr;
}

#include "FwupdSourcesBackend.moc"
#include "moc_FwupdSourcesBackend.cpp"
