#include "qml_material/item/round_item.hpp"

#include <QSGGeometry>

using namespace qml_material;

namespace
{
void updateGeometry(QSGGeometry *g, double radius_, QRectF rect) {
    int vertexCount = 0;

    // Radius should never exceeds half of the width or half of the height
    qreal  radius = qMin(qMin(rect.width() / 2, rect.height() / 2), radius_);
    rect.adjust(radius, radius, -radius, -radius);

    int segments = qMin(30, qCeil(radius)); // Number of segments per corner.

    g->allocate((segments + 1) * 4);

    QVector2D* vertices = (QVector2D*)g->vertexData();

    for (int part = 0; part < 2; ++part) {
        for (int i = 0; i <= segments; ++i) {
            // ### Should change to calculate sin/cos only once.
            qreal angle = qreal(0.5 * M_PI) * (part + i / qreal(segments));
            qreal s     = qFastSin(angle);
            qreal c     = qFastCos(angle);
            qreal y =
                (part ? rect.bottom() : rect.top()) - radius * c; // current inner y-coordinate.
            qreal lx = rect.left() - radius * s;  // current inner left x-coordinate.
            qreal rx = rect.right() + radius * s; // current inner right x-coordinate.

            vertices[vertexCount++] = QVector2D(rx, y);
            vertices[vertexCount++] = QVector2D(lx, y);
        }
    }
}
} // namespace

RoundItem::RoundItem(QQuickItem* parent): QQuickItem(parent) {}

RoundItem::~RoundItem() {}

QSGNode* RoundItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    return nullptr;
}

#include <qml_material/item/moc_round_item.cpp>