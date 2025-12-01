#pragma once
#include <QSGGeometry>
#include <QColor>
#include <QVector4D>

#include "qml_material/core.hpp"

namespace qml_material::sg
{

struct BasicVertex {
    // position
    float x;
    float y;

    // color
    float r;
    float g;
    float b;
    float a;

    void set_point(float x, float y) noexcept {
        this->x = x;
        this->y = y;
    }
    void set_point(QVector2D v) noexcept {
        x = v.x();
        y = v.y();
    }
    void set_color(QRgb c) noexcept {
        r = qRed(c) / 255.0f;
        g = qGreen(c) / 255.0f;
        b = qBlue(c) / 255.0f;
        a = qAlpha(c) / 255.0f;
    }
    void set_color(const QColor& c) noexcept { set_color(c.rgb()); }
};

struct RectangleVertex : BasicVertex {
    // circle edge
    float ce_x;                  // if in x edge
    float ce_y;                  // if in y edge
    float ce_distance_to_outter; // from inner rect
    float ce_distance_to_inner;  // from inner rect

    operator QVector2D() const { return { x, y }; }
    operator QColor() const { return QColor::fromRgbF(r, g, b, a); }
};
auto create_rectangle_geometry() -> up<QSGGeometry>;
// tl tr bl br
void update_rectangle_geometry(RectangleVertex* vertexs, QVector2D size, QRgb color,
                               QVector4D radius);

struct ShadowVertex : BasicVertex {
    // shadow
    float offset_x;
    float offset_y;
    float distance_correction;

    void set_offset(QVector2D v) noexcept {
        offset_x = v.x();
        offset_y = v.y();
    }
};
auto create_shadow_geometry() -> up<QSGGeometry>;

enum ShadowFlags
{
    None_ShadowFlag = 0x00,
    /** The occluding object is not opaque. Knowing that the occluder is opaque allows
     * us to cull shadow geometry behind it and improve performance. */
    TransparentOccluder_ShadowFlag = 0x01,
    /** Don't try to use analytic shadows. */
    GeometricOnly_ShadowFlag = 0x02,
    /** Light position represents a direction, light radius is blur radius at elevation 1 */
    DirectionalLight_ShadowFlag = 0x04,
    /** Concave paths will only use blur to generate the shadow */
    ConcaveBlurOnly_ShadowFlag = 0x08,
    /** mask for all shadow flags */
    All_ShadowFlag = 0x0F
};
struct ShadowParams {
    QVector3D z_plane_params;
    QVector3D light_pos;
    float     light_radius { 0 };
    QRgb      ambient_color { 0 };
    QRgb      spot_color { 0 };
    uint32_t  flags { 0 };
    QVector4D radius;
};
void update_shadow_geometry(QSGGeometry* geo, const ShadowParams& params, const QRectF& rect);

} // namespace qml_material::sg