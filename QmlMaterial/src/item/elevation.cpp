#include "qml_material/item/elevation.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>

#include "qml_material/scenegraph/geometry.h"
#include "qml_material/scenegraph/elevation_material.h"

// some parms comes from
// https://github.com/flutter/engine/blob/3.24.3/display_list/skia/dl_sk_dispatcher.cc#L295
// https://github.com/google/skia/blob/canvaskit/0.38.2/src/gpu/ganesh/SurfaceDrawContext.cpp#L1077
constexpr float     kAmbientAlpha      = 0.039f;
constexpr float     kSpotAlpha         = 0.25f;
constexpr float     kShadowLightRadius = 800.0f;
constexpr float     kShadowLightHeight = 600.0f;
constexpr QVector3D kLightPos          = { 0, -1, 1 };

namespace qml_material
{
namespace sg
{
class ElevationNode : public QSGGeometryNode {
public:
    ElevationNode() {
        setGeometry(create_shadow_geometry().release());
        setMaterial(new ElevationMaterial {});
        setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    }

    void init(QQuickItem* item) {
        static_cast<ElevationMaterial*>(material())->init_fadeoff_texture(item->window());
    }

    void updateGeometry() {
        // auto             vertices = static_cast<ShadowVertex*>(geometry()->vertexData());
        sg::ShadowParams params;
        {
            params.z_plane_params = QVector3D(0, 0, elevation);
            params.light_pos      = kLightPos;
            params.light_radius   = kShadowLightRadius / kShadowLightHeight;
            params.radius         = radius;
            if (elevation == 0) {
                params.flags |= sg::ShadowFlags::TransparentOccluder_ShadowFlag;
            }
            params.flags |= sg::ShadowFlags::DirectionalLight_ShadowFlag;
            auto c = this->color;
            c.setAlphaF(kAmbientAlpha * this->color.alphaF());
            params.ambient_color = c.rgba();
            c.setAlphaF(kSpotAlpha * this->color.alphaF());
            params.spot_color = c.rgba();

            for (int i = 0; i < 4; i++) {
                params.radius[i] = std::min<float>(params.radius[i], rect.height() / 2.0f);
            }
        }
        update_shadow_geometry(geometry(), params, rect);

        markDirty(QSGNode::DirtyGeometry);
    }

    double    elevation;
    QRectF    rect;
    QColor    color;
    QVector4D radius;
};
} // namespace sg

Elevation::Elevation(QQuickItem* parentItem)
    : QQuickItem(parentItem), m_elevation(0), m_corners(), m_radius(0), m_color(Qt::black) {
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &Elevation::elevationChanged, this, &Elevation::update);
    connect(this, &Elevation::colorChanged, this, &Elevation::update);
    connect(this, &Elevation::cornersChanged, this, &Elevation::update);
}

Elevation::~Elevation() {}

auto Elevation::elevation() const -> double { return m_elevation; }
void Elevation::setelevation(double l) {
    if (! qFuzzyCompare(l, m_elevation)) {
        m_elevation = l;
        elevationChanged(m_elevation);
    }
}
auto Elevation::corners() const -> const CornersGroup& { return m_corners; }
void Elevation::setCorners(const CornersGroup& c) {
    m_corners = c;
    cornersChanged(c);
}

qreal Elevation::radius() const { return m_radius; }

void Elevation::setRadius(qreal newRadius) {
    if (qFuzzyCompare(m_radius, newRadius)) {
        return;
    }
    m_radius = newRadius;
    setCorners(m_radius);
    radiusChanged(m_radius);
}

QColor Elevation::color() const { return m_color; }

void Elevation::setColor(const QColor& newColor) {
    if (newColor == m_color) {
        return;
    }

    m_color = newColor;
    colorChanged(m_color);
}

void Elevation::componentComplete() { QQuickItem::componentComplete(); }

void Elevation::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData& value) {
    if (change == QQuickItem::ItemSceneChange && value.window) {
        // checkSoftwareItem();
    }

    QQuickItem::itemChange(change, value);
}

QSGNode* Elevation::updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) {
    Q_UNUSED(data);

    if (boundingRect().isEmpty()) {
        delete node;
        return nullptr;
    }
    auto shadowNode = static_cast<sg::ElevationNode*>(node);

    if (! shadowNode) {
        shadowNode = new sg::ElevationNode {};
        shadowNode->init(this);
    }
    shadowNode->rect      = boundingRect();
    shadowNode->elevation = m_elevation;
    shadowNode->radius    = m_corners.toVector4D();
    shadowNode->color     = m_color.rgb();
    shadowNode->updateGeometry();
    return shadowNode;
}
} // namespace qml_material

#include "qml_material/item/moc_elevation.cpp"