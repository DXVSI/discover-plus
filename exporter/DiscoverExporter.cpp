/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DiscoverExporter.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <chrono>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>

using namespace std::chrono_literals;

DiscoverExporter::DiscoverExporter()
    : QObject(nullptr)
    , m_excludedProperties({"executables", "canExecute"})
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &DiscoverExporter::fetchResources);
}

DiscoverExporter::~DiscoverExporter() = default;

void DiscoverExporter::setExportPath(const QUrl &url)
{
    m_path = url;
}

QJsonObject itemDataToMap(const AbstractResource *res, const QSet<QByteArray> &excluded)
{
    QJsonObject ret;
    int propsCount = res->metaObject()->propertyCount();
    for (int i = 0; i < propsCount; i++) {
        QMetaProperty prop = res->metaObject()->property(i);
        if (prop.userType() >= QMetaType::User || excluded.contains(prop.name()))
            continue;

        const QVariant val = prop.read(res);
        if (val.isNull())
            continue;

        ret.insert(QLatin1String(prop.name()), QJsonValue::fromVariant(val));
    }
    return ret;
}

void DiscoverExporter::fetchResources()
{
    ResourcesModel *m = ResourcesModel::global();
    QSet<ResultsStream *> streams;
    const auto backends = m->backends();
    for (auto backend : backends) {
        streams << backend->search({});
    }
    auto stream = new AggregatedResultsStream(streams);
    connect(stream, &AggregatedResultsStream::finished, this, &DiscoverExporter::exportResources);
}

void DiscoverExporter::exportResources(const QVector<StreamResult> &resources)
{
    QJsonArray data;
    for (auto res : resources) {
        data += itemDataToMap(res.resource, m_excludedProperties);
    }

    QJsonDocument doc = QJsonDocument(data);
    if (doc.isNull()) {
        qWarning() << "Could not completely export the data to " << m_path;
        return;
    }

    QFile f(m_path.toLocalFile());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        int w = f.write(doc.toJson(QJsonDocument::Indented));
        if (w <= 0)
            qWarning() << "Could not completely export the data to " << m_path;
    } else {
        qWarning() << "Could not write to " << m_path;
    }
    qDebug() << "exported items: " << data.count() << " to " << m_path;
    Q_EMIT exportDone();
}

#include "moc_DiscoverExporter.cpp"
