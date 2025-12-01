#include "qml_material/util/corner.hpp"

using namespace qml_material;

CornersGroup::CornersGroup() noexcept: CornersGroup(0) {}
CornersGroup::CornersGroup(qreal r) noexcept: CornersGroup(r, r, r, r) {}
CornersGroup::CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft,
                           qreal topLeft) noexcept
    : m_bottomRight(bottomRight),
      m_topRight(topRight),
      m_bottomLeft(bottomLeft),
      m_topLeft(topLeft) {}
CornersGroup::~CornersGroup() {}

qreal CornersGroup::topLeft() const noexcept { return m_topLeft; }

auto CornersGroup::radius() const noexcept -> qreal {
    // return max
    return std::max({ m_topLeft, m_topRight, m_bottomLeft, m_bottomRight });
}
void CornersGroup::setRadius(qreal v) noexcept {
    m_topLeft     = v;
    m_topRight    = v;
    m_bottomLeft  = v;
    m_bottomRight = v;
}
void CornersGroup::setTopLeft(qreal newTopLeft) noexcept { m_topLeft = newTopLeft; }

qreal CornersGroup::topRight() const noexcept { return m_topRight; }

void CornersGroup::setTopRight(qreal newTopRight) noexcept { m_topRight = newTopRight; }

qreal CornersGroup::bottomLeft() const noexcept { return m_bottomLeft; }

void CornersGroup::setBottomLeft(qreal newBottomLeft) noexcept { m_bottomLeft = newBottomLeft; }

qreal CornersGroup::bottomRight() const noexcept { return m_bottomRight; }

void CornersGroup::setBottomRight(qreal newBottomRight) noexcept { m_bottomRight = newBottomRight; }

QVector4D CornersGroup::toVector4D() const noexcept {
    return QVector4D {
        (float)m_topLeft, (float)m_topRight, (float)m_bottomLeft, (float)m_bottomRight
    };
}
CornersGroup::operator QVector4D() const noexcept { return toVector4D(); }

#include <qml_material/util/moc_corner.cpp>