#include "qml_material/item/rectangle.hpp"

#include <QQuickWindow>
#include <QSGRectangleNode>
#include <QSGRendererInterface>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>

#include "qml_material/scenegraph/geometry.h"
#include "qml_material/scenegraph/rectangle_material.h"

namespace qml_material
{
namespace sg
{
class RectangleNode : public QSGGeometryNode {
public:
    RectangleNode() {
        setGeometry(create_rectangle_geometry().release());
        setMaterial(new RectangleMaterial {});
        setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    }

    void updateGeometry() {
        auto vertices = static_cast<RectangleVertex*>(geometry()->vertexData());

        for (int i = 0; i < 4; i++) {
            radius[i] = std::min<float>(radius[i], rect.size().height() / 2.0f);
        }
        QVector2D size = { (float)rect.size().width(), (float)rect.size().height() };
        update_rectangle_geometry(vertices, size, color, radius);
        markDirty(QSGNode::DirtyGeometry);
    }

    QRectF    rect;
    QRgb      color { 0 };
    QVector4D radius;
};
} // namespace sg

Rectangle::Rectangle(QQuickItem* parentItem)
    : QQuickItem(parentItem), m_corners(), m_radius(0), m_color(Qt::transparent) {
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &Rectangle::colorChanged, this, &Rectangle::update);
    connect(this, &Rectangle::cornersChanged, this, &Rectangle::update);
}

Rectangle::~Rectangle() {}

auto Rectangle::corners() const -> CornersGroup { return m_corners; }
void Rectangle::setCorners(const CornersGroup& c) {
    m_corners = c;
    cornersChanged();
}

qreal Rectangle::radius() const { return m_radius; }

void Rectangle::setRadius(qreal newRadius) {
    if (qFuzzyCompare(m_radius, newRadius)) {
        return;
    }
    m_radius = newRadius;
    setCorners(m_radius);
    radiusChanged();
}

QColor Rectangle::color() const { return m_color; }

void Rectangle::setColor(const QColor& newColor) {
    if (newColor == m_color) {
        return;
    }

    m_color = newColor;
    colorChanged();
}

void Rectangle::componentComplete() { QQuickItem::componentComplete(); }

void Rectangle::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData& value) {
    if (change == QQuickItem::ItemSceneChange && value.window) {
        // checkSoftwareItem();
    }

    QQuickItem::itemChange(change, value);
}

QSGNode* Rectangle::updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) {
    Q_UNUSED(data);

    if (boundingRect().isEmpty()) {
        delete node;
        return nullptr;
    }

    auto shadowNode = static_cast<sg::RectangleNode*>(node);
    if (! shadowNode) {
        shadowNode = new sg::RectangleNode {};
    }
    shadowNode->rect   = boundingRect();
    shadowNode->radius = m_corners.toVector4D();
    shadowNode->color  = m_color.rgba();
    shadowNode->updateGeometry();
    return shadowNode;
}
} // namespace qml_material

#include "qml_material/item/moc_rectangle.cpp"