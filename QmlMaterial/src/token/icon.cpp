#include "qml_material/token/icon.hpp"

namespace qml_material
{
auto token::create_icon_token(QObject* parent) -> IconToken* { return new IconToken(parent); }
} // namespace qml_material

#include "qml_material/token/moc_icon.cpp"