#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QTimer>
#include <QtQml/QQmlEngine>

#include "qml_material/enum.hpp"

namespace qml_material
{
struct SnakeModelItem {
    Q_GADGET

    Q_PROPERTY(qint32 sid MEMBER id FINAL)
    Q_PROPERTY(QString text MEMBER text FINAL)
    Q_PROPERTY(qint32 duration MEMBER duration FINAL)
    Q_PROPERTY(Enum::ToastFlags flag MEMBER flag FINAL)
    Q_PROPERTY(QObject* action MEMBER action FINAL)
public:
    qint32           id { 0 };
    QString          text {};
    qint32           duration { 0 };
    Enum::ToastFlags flag {};
    QObject*         action { nullptr };
};

class SnakeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    using iterator = std::vector<SnakeModelItem>::iterator;

    SnakeModel(QObject* parent = nullptr);
    virtual ~SnakeModel();

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE SnakeModelItem createSnake();

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    auto findById(qint32 id) -> iterator;

    Q_SLOT void showSnake(const SnakeModelItem&);
    Q_SLOT void removeById(qint32 id);

private:
    Q_SLOT void onRemovePriv(const QModelIndex& parent, int first, int last);
    qint32      removeExtra(qint32 id);

    std::int32_t                m_snake_id;
    std::vector<SnakeModelItem> m_items;
    std::vector<SnakeModelItem> m_saved_items;

    struct Extra {
        QTimer* timer { nullptr };
    };
    std::unordered_map<qint32, Extra> m_extra;
};
} // namespace qml_material