#pragma once

#include <QQmlEngine>
#include <QQuickItem>

#include "qml_material/util/corner.hpp"

namespace qml_material
{

class Rectangle : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(RectangleImpl)

    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(qml_material::CornersGroup corners READ corners WRITE setCorners NOTIFY cornersChanged FINAL)
public:
    Rectangle(QQuickItem* parent = nullptr);
    ~Rectangle() override;

    auto          corners() const -> CornersGroup;
    void          setCorners(const CornersGroup&);
    Q_SIGNAL void cornersChanged();

    qreal         radius() const;
    void          setRadius(qreal newRadius);
    Q_SIGNAL void radiusChanged();

    QColor        color() const;
    void          setColor(const QColor& newColor);
    Q_SIGNAL void colorChanged();

    void componentComplete() override;

protected:
    void     itemChange(QQuickItem::ItemChange            change,
                        const QQuickItem::ItemChangeData& value) override;
    QSGNode* updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) override;

private:
    CornersGroup m_corners;
    qreal        m_radius;
    QColor       m_color;
};
} // namespace qml_material