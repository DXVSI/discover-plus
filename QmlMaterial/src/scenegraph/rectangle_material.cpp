#include "qml_material/scenegraph/rectangle_material.h"

namespace qml_material::sg
{

RectangleMaterial::RectangleMaterial() { setFlag(QSGMaterial::Blending, true); }

QSGMaterialShader* RectangleMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new RectangleShader {};
}

QSGMaterialType* RectangleMaterial::type() const {
    static QSGMaterialType staticType;
    return &staticType;
}

int RectangleMaterial::compare(const QSGMaterial* other) const {
    auto material = static_cast<const RectangleMaterial*>(other);
    return QSGMaterial::compare(other);
}

RectangleShader::RectangleShader() {
    setShaderFileName(QSGMaterialShader::VertexStage,
                      QLatin1String(":/Qcm/Material/assets/shader/rectangle.vert.qsb"));
    setShaderFileName(QSGMaterialShader::FragmentStage,
                      QLatin1String(":/Qcm/Material/assets/shader/rectangle.frag.qsb"));
}

bool RectangleShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial,
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
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }
    return changed;
}
} // namespace qml_material::sg