#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QTimer>
#include <QtQml/QQmlEngine>

#include "qml_material/enum.hpp"

namespace qml_material
{

class PaletteModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    PaletteModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
} // namespace qml_material