#include "qml_material/scenegraph/elevation_material.h"

#include <mutex>
#include <QImage>
#include <QQuickWindow>
#include <QSGTexture>
#include "qml_material/scenegraph/skia_shadow.h"

namespace qml_material::sg
{

ElevationMaterial::ElevationMaterial(): m_fadeoff_texture(nullptr) {
    setFlag(QSGMaterial::Blending, true);
}
ElevationMaterial::~ElevationMaterial() { delete m_fadeoff_texture; }

QSGMaterialShader* ElevationMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new ElevationShader {};
}

QSGMaterialType* ElevationMaterial::type() const {
    static QSGMaterialType staticType;
    return &staticType;
}

int ElevationMaterial::compare(const QSGMaterial* other) const {
    auto material = static_cast<const ElevationMaterial*>(other);
    return QSGMaterial::compare(other);
}

auto ElevationMaterial::strength_texture() -> QSGTexture* { return m_fadeoff_texture; }
void ElevationMaterial::init_fadeoff_texture(QQuickWindow* win) {
    const int width  = 128;
    const int height = 1;
    // Create a grayscale image (R8 equivalent)
    QImage image(width, height, QImage::Format_Grayscale8);

    // Fill the image with values for the red channel (e.g., gradient)
    uchar* row = image.scanLine(0);
    for (int i = 0; i < 128; i++) {
        float d = sk::SK_Scalar1 - i / 127.0f;
        row[i]  = sk::SkScalarRoundToInt(((float)std::exp(-4 * d * d) - 0.018f) * 255);
    }

    // Create the texture from the grayscale image
    QSGTexture* texture = win->createTextureFromImage(image);
    m_fadeoff_texture   = texture;
}

ElevationShader::ElevationShader() {
    setShaderFileName(QSGMaterialShader::VertexStage,
                      QLatin1String(":/Qcm/Material/assets/shader/shadow.vert.qsb"));
    setShaderFileName(QSGMaterialShader::FragmentStage,
                      QLatin1String(":/Qcm/Material/assets/shader/shadow.frag.qsb"));
}

bool ElevationShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial,
                                        QSGMaterial* oldMaterial) {
    Q_UNUSED(newMaterial);
    Q_UNUSED(oldMaterial);
    bool        changed = false;
    QByteArray* buf     = state.uniformData();
    Q_ASSERT(buf->size() >= 68);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = std::pow(state.opacity(), 3);
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }
    return changed;
}

void ElevationShader::updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                                         QSGMaterial* newMaterial, QSGMaterial*) {
    auto mat = static_cast<ElevationMaterial*>(newMaterial);
    if (binding == 1) {
        *texture = mat->strength_texture();
        texture[0]->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    }
}

} // namespace qml_material::sg