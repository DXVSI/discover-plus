#pragma once

#include <QSGMaterial>

namespace qml_material::sg
{

class RectangleMaterial : public QSGMaterial {
public:
    RectangleMaterial();

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;
};

class RectangleShader : public QSGMaterialShader {
public:
    RectangleShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;

};

} // namespace qml_material::sg