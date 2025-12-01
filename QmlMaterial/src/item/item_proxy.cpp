#include "qml_material/item/item_proxy.hpp"

namespace qml_material
{
ItemProxy::ItemProxy(QQuickItem* parent): QQuickItem(parent), m_visible_on_item(true) {}
ItemProxy::~ItemProxy() {}

auto ItemProxy::item() const -> QObject* { return m_item; }
auto ItemProxy::visibleOnItem() const -> bool { return m_visible_on_item; }

void ItemProxy::setItem(QObject* o) {
    if (m_item != o) {
        if (m_item) {
            if (auto p = qobject_cast<QQuickItem*>(m_item)) {
                p->setParentItem(nullptr);
            }
            m_item->deleteLater();
        }
        m_item = o;
        if (m_item) {
            if (auto p = qobject_cast<QQuickItem*>(o)) {
                p->setParentItem(this);
            } else {
                m_item->setParent(this);
            }
        }
        if (visibleOnItem()) {
            setVisible(m_item);
        }
        itemChanged();
    }
}
void ItemProxy::setVisibleOnItem(bool v) {
    if (m_visible_on_item != v) {
        m_visible_on_item = v;
        visibleOnItemChanged();
    }
}

} // namespace qml_material

#include "qml_material/item/moc_item_proxy.cpp"
