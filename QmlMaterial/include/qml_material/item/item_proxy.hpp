#pragma once

#include <QtCore/QPointer>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

namespace qml_material
{
class ItemProxy : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool visibleOnItem READ visibleOnItem WRITE setVisibleOnItem NOTIFY visibleOnItemChanged FINAL)
public:
    ItemProxy(QQuickItem* parent = nullptr);
    ~ItemProxy();

    auto item() const -> QObject*;
    auto visibleOnItem() const -> bool;

    Q_SLOT void setItem(QObject*);
    Q_SLOT void setVisibleOnItem(bool);

    Q_SIGNAL void itemChanged();
    Q_SIGNAL void visibleOnItemChanged();

private:
    QPointer<QObject> m_item;
    bool m_visible_on_item;
};

} // namespace qml_material