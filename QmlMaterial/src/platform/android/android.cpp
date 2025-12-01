#include "qml_material/util/qml_util.hpp"
#include "qml_material/token/color.hpp"

namespace qml_material
{
void sysNotifyInit(MdColorMgr&) {}
auto sysColorScheme() -> Enum::ThemeMode { return {}; }
auto sysAccentColor() -> QColor { return {}; }

} // namespace qml_material