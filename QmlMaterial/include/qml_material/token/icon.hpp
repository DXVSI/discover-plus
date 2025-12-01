#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtGui/QColor>
#include <QtQml/QQmlEngine>


namespace qml_material::token
{
/**
 * @brief IconToken, full @ref icon_code.inl "codes"
 * 
 */
class IconToken : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    using QObject::QObject;

#define CODE(name, value)                                      \
    Q_PROPERTY(QString name MEMBER code_##name CONSTANT FINAL) \
    QString code_##name { QString::fromUtf8(value) }

#include "qml_material/token/icon_code.inl"

#undef CODE
};

auto create_icon_token(QObject* parent) -> IconToken*;
} // namespace qml_material::token