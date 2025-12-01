#include "qml_material/util/qml_util.hpp"
#include "qml_material/token/color.hpp"

#include <emscripten/val.h>
#include <emscripten/bind.h>

namespace
{
bool is_dark_mode() {
    auto window = emscripten::val::global("window");
    auto media_query =
        window.call<emscripten::val>("matchMedia", std::string("(prefers-color-scheme: dark)"));
    return media_query["matches"].as<bool>();
}
} // namespace
namespace qml_material
{
void sysNotifyInit(MdColorMgr&) {}
auto sysColorScheme() -> Enum::ThemeMode {
    if (is_dark_mode()) return Enum::ThemeMode::Dark;
    return Enum::ThemeMode::Light;
}
auto sysAccentColor() -> QColor { return {}; }

} // namespace qml_material