/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ApplicationAddonsModel.h"
#include "utils.h"
#include <Transaction/TransactionModel.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>

ApplicationAddonsModel::ApplicationAddonsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_app(nullptr)
{
    // new QAbstractItemModelTester(this, this);

    connect(TransactionModel::global(), &TransactionModel::transactionRemoved, this, &ApplicationAddonsModel::transactionOver);
    connect(ResourcesModel::global(), &ResourcesModel::resourceDataChanged, this, [this](AbstractResource *resource, const QVector<QByteArray> &properties) {
        if (!properties.contains("state")) {
            return;
        }

        const QString appstreamId = resource->appstreamId();
        if (std::ranges::any_of(m_initial, [&appstreamId](const PackageState &state) {
                return appstreamId == state.packageName();
            })) {
            resetState();
        }
    });
}

QHash<int, QByteArray> ApplicationAddonsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(Qt::CheckStateRole, "checked");
    roles.insert(PackageNameRole, "packageName");
    return roles;
}

void ApplicationAddonsModel::setApplication(AbstractResource *app)
{
    if (app == m_app) {
        return;
    }

    if (m_app) {
        disconnect(m_app, nullptr, this, nullptr);
    }

    m_app = app;
    resetState();
    if (m_app) {
        connect(m_app, &QObject::destroyed, this, [this]() {
            setApplication(nullptr);
        });
    }
    Q_EMIT applicationChanged();
}

void ApplicationAddonsModel::resetState()
{
    beginResetModel();
    m_state.clear();
    m_initial = m_app ? m_app->addonsInformation() : QList<PackageState>();
    endResetModel();

    Q_EMIT stateChanged();
}

AbstractResource *ApplicationAddonsModel::application() const
{
    return m_app;
}

int ApplicationAddonsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_initial.size();
}

QVariant ApplicationAddonsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_initial.size()) {
        return QVariant();
    }

    const auto &packageState = m_initial[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return packageState.name();
    case Qt::ToolTipRole:
        return packageState.description();
    case PackageNameRole:
        return packageState.packageName();
    case Qt::CheckStateRole: {
        const AddonList::State state = m_state.addonState(packageState.name());
        if (state == AddonList::None) {
            return packageState.isInstalled() ? Qt::Checked : Qt::Unchecked;
        } else {
            return state == AddonList::ToInstall ? Qt::Checked : Qt::Unchecked;
        }
    }
    }

    return QVariant();
}

void ApplicationAddonsModel::discardChanges()
{
    // dataChanged should suffice, but it doesn't
    beginResetModel();
    m_state.clear();
    Q_EMIT stateChanged();
    endResetModel();
}

void ApplicationAddonsModel::applyChanges()
{
    ResourcesModel::global()->installApplication(m_app, m_state);
}

void ApplicationAddonsModel::changeState(const QString &packageName, bool installed)
{
    auto it = std::find_if(m_initial.constBegin(), m_initial.constEnd(), [packageName](const auto &packageState) {
        return packageState.packageName() == packageName;
    });
    Q_ASSERT(it != m_initial.constEnd());
    const auto &packageState = *it;

    const bool restored = packageState.isInstalled() == installed;

    if (restored) {
        m_state.resetAddon(packageName);
    } else {
        m_state.addAddon(packageName, installed);
    }

    Q_EMIT stateChanged();
}

bool ApplicationAddonsModel::hasChanges() const
{
    return !m_state.isEmpty();
}

bool ApplicationAddonsModel::isEmpty() const
{
    return m_initial.isEmpty();
}

void ApplicationAddonsModel::transactionOver(Transaction *t)
{
    if (t->resource() != m_app) {
        return;
    }

    resetState();
}

#include "moc_ApplicationAddonsModel.cpp"
