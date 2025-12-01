#pragma once
#include <QQmlEngine>
#include <QVector4D>

namespace qml_material
{
class CornersGroup {
    Q_GADGET
    QML_VALUE_TYPE(corners)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(qreal topLeft READ topLeft WRITE setTopLeft FINAL)
    Q_PROPERTY(qreal topRight READ topRight WRITE setTopRight FINAL)
    Q_PROPERTY(qreal bottomLeft READ bottomLeft WRITE setBottomLeft FINAL)
    Q_PROPERTY(qreal bottomRight READ bottomRight WRITE setBottomRight FINAL)

public:
    Q_INVOKABLE CornersGroup() noexcept;
    Q_INVOKABLE CornersGroup(qreal) noexcept;
    Q_INVOKABLE CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft,
                             qreal topLeft) noexcept;
    ~CornersGroup();

    CornersGroup(const CornersGroup&)            = default;
    CornersGroup(CornersGroup&&)                 = default;
    CornersGroup& operator=(const CornersGroup&) = default;
    CornersGroup& operator=(CornersGroup&&)      = default;

    Q_INVOKABLE qreal radius() const noexcept;
    void              setRadius(qreal) noexcept;

    qreal topLeft() const noexcept;
    void  setTopLeft(qreal newTopLeft) noexcept;

    qreal topRight() const noexcept;
    void  setTopRight(qreal newTopRight) noexcept;

    qreal bottomLeft() const noexcept;
    void  setBottomLeft(qreal newBottomLeft) noexcept;

    qreal bottomRight() const noexcept;
    void  setBottomRight(qreal newBottomRight) noexcept;

    Q_INVOKABLE QVector4D toVector4D() const noexcept;
    operator QVector4D() const noexcept;

private:
    double m_bottomRight;
    double m_topRight;
    double m_bottomLeft;
    double m_topLeft;
};

} // namespace qml_material