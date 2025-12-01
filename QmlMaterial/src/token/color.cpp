#include "qml_material/token/color.hpp"

#include <QtGui/QGuiApplication>
#include <QStyleHints>

#include "qml_material/helper.hpp"
#include "qml_material/util/qml_util.hpp"

using namespace qcm;
using namespace qml_material;

namespace
{
constexpr QRgb BASE_COLOR { qRgb(190, 231, 253) };

auto gen_on_map(const MdScheme& sh) -> std::map<QColor, QColor, QColorCompare> {
    std::map<QColor, QColor, QColorCompare> map {
        { sh.primary, sh.on_primary },
        { sh.primary_container, sh.on_primary_container },
        { sh.secondary, sh.on_secondary },
        { sh.secondary_container, sh.on_secondary_container },
        { sh.tertiary, sh.on_tertiary },
        { sh.tertiary_container, sh.on_tertiary_container },
        { sh.error, sh.on_error },
        { sh.error_container, sh.on_error_container },

        { sh.inverse_surface, sh.inverse_on_surface },
        { sh.inverse_primary, sh.inverse_on_surface },

        { sh.background, sh.on_background },
        { sh.surface, sh.on_surface },
        { sh.surface_variant, sh.on_surface_variant },
        { sh.surface_dim, sh.on_surface },
        { sh.surface_bright, sh.on_surface },
        { sh.surface_container, sh.on_surface },
        { sh.surface_container_high, sh.on_surface },
        { sh.surface_container_highest, sh.on_surface },
        { sh.surface_container_low, sh.on_surface },
        { sh.surface_container_lowest, sh.on_surface },

        { sh.outline, sh.background },
        { sh.shadow, sh.background },

        { sh.surface_1, sh.on_surface },
        { sh.surface_2, sh.on_surface },
        { sh.surface_3, sh.on_surface },
        { sh.surface_4, sh.on_surface },
        { sh.surface_5, sh.on_surface },
    };
    auto map2 = map;
    for (auto& el : map2) {
        map.insert({ el.second, el.first });
    }
    return map;
}
} // namespace

MdColorMgr::MdColorMgr(QObject* parent)
    : QObject(parent),
      m_accent_color(BASE_COLOR),
      m_mode(sysColorScheme()),
      m_last_mode(m_mode),
      m_scheme_type(Enum::PaletteType::PaletteNeutral),
      m_use_sys_color_scheme(true),
      m_use_sys_accent_color(false) {
    genScheme();
    connect(this, &Self::modeChanged, this, &Self::genScheme);
    connect(this, &Self::accentColorChanged, this, &Self::genScheme);
    connect(this, &Self::paletteTypeChanged, this, &Self::genScheme);

    sysNotifyInit(*this);

    connect(this, &Self::useSysColorSMChanged, this, &Self::refrehFromSystem);
    connect(this, &Self::useSysAccentColorChanged, this, &Self::refrehFromSystem);
}

Enum::ThemeMode MdColorMgr::sysColorScheme() const { return ::sysColorScheme(); }
QColor          MdColorMgr::sysAccentColor() const { return ::sysAccentColor(); }

Enum::ThemeMode MdColorMgr::mode() const { return m_mode; }
void            MdColorMgr::setMode(Enum::ThemeMode v) {
    if (std::exchange(m_mode, v) != v) {
        Q_EMIT modeChanged(m_mode);
    }
}

QColor MdColorMgr::accentColor() const { return m_accent_color; }

bool MdColorMgr::useSysColorSM() const { return m_use_sys_color_scheme; };
bool MdColorMgr::useSysAccentColor() const { return m_use_sys_accent_color; };
auto MdColorMgr::paletteType() const -> Enum::PaletteType { return m_scheme_type; }
void MdColorMgr::setPaletteType(Enum::PaletteType t) {
    if (std::exchange(m_scheme_type, t) != t) {
        paletteTypeChanged();
    }
}

void MdColorMgr::setAccentColor(QColor v) {
    if (std::exchange(m_accent_color, v) != v) {
        Q_EMIT accentColorChanged(m_accent_color);
    }
}

void MdColorMgr::setUseSysColorSM(bool v) {
    if (std::exchange(m_use_sys_color_scheme, v) != v) {
        Q_EMIT useSysColorSMChanged();
    }
}

void MdColorMgr::setUseSysAccentColor(bool v) {
    if (std::exchange(m_use_sys_accent_color, v) != v) {
        Q_EMIT useSysAccentColorChanged();
    }
}

QColor MdColorMgr::getOn(QColor in) const {
    if (m_on_map.contains(in)) {
        return m_on_map.at(in);
    }
    return m_scheme.on_background;
}

void MdColorMgr::genSchemeImpl(Enum::ThemeMode mode) {
    if (mode == Enum::ThemeMode::Light)
        m_scheme = material_light_color_scheme(m_accent_color.rgb(), paletteType());
    else
        m_scheme = material_dark_color_scheme(m_accent_color.rgb(), paletteType());

    m_last_mode = mode;

    m_on_map = gen_on_map(m_scheme);
    Q_EMIT schemeChanged();
}

void MdColorMgr::genScheme() {
    auto m = useSysColorSM() ? sysColorScheme() : mode();
    genSchemeImpl(m);
}

void MdColorMgr::refrehFromSystem() {
    if (useSysColorSM()) {
        genSchemeImpl(sysColorScheme());
    } else {
        if (m_last_mode != mode()) {
            genScheme();
        }
    }

    if (useSysAccentColor()) {
        setAccentColor(sysAccentColor());
    }
}

void MdColorMgr::selectFromImage(const QImage& image) {
    auto color = color_from_image(image);
    setUseSysAccentColor(false);
    setAccentColor(color);
}

#include <qml_material/token/moc_color.cpp>