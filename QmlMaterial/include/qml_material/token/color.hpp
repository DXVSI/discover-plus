#pragma once

#include <unordered_map>

#include <QtQml/QQmlEngine>
#include <QtGui/QColor>

#include "qml_material/core.hpp"
#include "qml_material/enum.hpp"
#include "qml_material/helper.hpp"

namespace qml_material
{

struct QColorCompare {
    using is_transparent = void;
    bool operator()(const QColor& a, const QColor& b) const noexcept { return a.rgba() < b.rgba(); }
};

/**
 * @brief Material Design [Color](https://m3.material.io/styles/color/roles) Manager class
 *
 * Manages color schemes and theme modes for Material Design implementation.
 * @image html color-role.png
 */
class MdColorMgr : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using Self = MdColorMgr;

    MdColorMgr(QObject* = nullptr);

    /**
     * @property mode
     * @brief Current theme mode (light/dark)
     */
    Q_PROPERTY(qml_material::Enum::ThemeMode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)

    /**
     * @property PaletteType
     * @brief Color scheme type
     */
    Q_PROPERTY(qml_material::Enum::PaletteType paletteType READ paletteType WRITE setPaletteType
                   NOTIFY paletteTypeChanged FINAL)

    /**
     * @property accentColor
     * @brief Primary accent color for the theme
     */
    Q_PROPERTY(
        QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged FINAL)

    /**
     * @property useSysColorSM
     * @brief Whether to use system color scheme mode
     */
    Q_PROPERTY(bool useSysColorSM READ useSysColorSM WRITE setUseSysColorSM NOTIFY
                   useSysColorSMChanged FINAL)

    /**
     * @property useSysAccentColor
     * @brief Whether to use system accent color
     */
    Q_PROPERTY(bool useSysAccentColor READ useSysAccentColor WRITE setUseSysAccentColor NOTIFY
                   useSysAccentColorChanged FINAL)

#define X(_n_)                                           \
    Q_PROPERTY(QColor _n_ READ _n_ NOTIFY schemeChanged) \
    QColor _n_() const { return m_scheme._n_; }

    /** @brief The primary color of the theme */
    X(primary)
    /** @brief Color used for content (text/icons) on primary color */
    X(on_primary)
    /** @brief A softer background color related to primary */
    X(primary_container)
    /** @brief Color for content on primary container */
    X(on_primary_container)
    /** @brief The secondary color used for less prominent components */
    X(secondary)
    /** @brief Color for content on secondary color */
    X(on_secondary)
    /** @brief A softer background color related to secondary */
    X(secondary_container)
    /** @brief Color for content on secondary container */
    X(on_secondary_container)
    /** @brief The tertiary color for specific emphasis */
    X(tertiary)
    /** @brief Color for content on tertiary color */
    X(on_tertiary)
    /** @brief A softer background color related to tertiary */
    X(tertiary_container)
    /** @brief Color for content on tertiary container */
    X(on_tertiary_container)
    /** @brief Color used for error states */
    X(error)
    /** @brief Color for content on error color */
    X(on_error)
    /** @brief A softer background color for error states */
    X(error_container)
    /** @brief Color for content on error container */
    X(on_error_container)
    /** @brief Main background color */
    X(background)
    /** @brief Color for content on background */
    X(on_background)
    /** @brief Surface color for cards, sheets, and menus */
    X(surface)
    /** @brief Color for content on surface */
    X(on_surface)
    /** @brief Alternative surface color */
    X(surface_variant)
    /** @brief Color for content on surface variant */
    X(on_surface_variant)
    /** @brief Color for borders and dividers */
    X(outline)
    /** @brief Subtle version of outline color */
    X(outline_variant)
    /** @brief Color used for shadows */
    X(shadow)
    /** @brief Color used for screen dimming */
    X(scrim)
    /** @brief Inverse of the surface color */
    X(inverse_surface)
    /** @brief Color for content on inverse surface */
    X(inverse_on_surface)
    /** @brief Inverse of the primary color */
    X(inverse_primary)
    /** @brief First elevation variant of surface color */
    X(surface_1)
    /** @brief Second elevation variant of surface color */
    X(surface_2)
    /** @brief Third elevation variant of surface color */
    X(surface_3)
    /** @brief Fourth elevation variant of surface color */
    X(surface_4)
    /** @brief Fifth elevation variant of surface color */
    X(surface_5)
    /** @brief Dimmed version of surface color */
    X(surface_dim)
    /** @brief Brightened version of surface color */
    X(surface_bright)
    /** @brief Base container variant of surface color */
    X(surface_container)
    /** @brief Low container variant of surface color */
    X(surface_container_low)
    /** @brief Lowest container variant of surface color */
    X(surface_container_lowest)
    /** @brief High container variant of surface color */
    X(surface_container_high)
    /** @brief Highest container variant of surface color */
    X(surface_container_highest)
#undef X

#define X(_n_) \
    Q_INVOKABLE QColor get_##_n_(double tone) const { return m_scheme._n_##_palette.get(tone); }

    X(neutral)
#undef X

#define X(_n_, _tone_)                                                         \
    Q_PROPERTY(QColor _n_##_##_tone_ READ _n_##_##_tone_ NOTIFY schemeChanged) \
    QColor _n_##_##_tone_() const { return m_scheme._n_##_palette.get(_tone_); }

    X(neutral, 10)
    X(neutral, 20)
    X(neutral, 30)
    X(neutral, 40)
    X(neutral, 50)
    X(neutral, 60)
    X(neutral, 70)
    X(neutral, 80)
    X(neutral, 90)
#undef X

public:
    auto mode() const -> Enum::ThemeMode;
    auto sysColorScheme() const -> Enum::ThemeMode;

    QColor sysAccentColor() const;
    QColor accentColor() const;
    bool   useSysColorSM() const;
    bool   useSysAccentColor() const;
    auto   paletteType() const -> Enum::PaletteType;
    void   setPaletteType(Enum::PaletteType);

    Q_INVOKABLE QColor getOn(QColor) const;

    Q_SLOT void setMode(Enum::ThemeMode);
    Q_SLOT void setAccentColor(QColor);
    Q_SLOT void setUseSysColorSM(bool);
    Q_SLOT void setUseSysAccentColor(bool);
    Q_SLOT void genScheme();
    Q_SLOT void refrehFromSystem();

    Q_SLOT void selectFromImage(const QImage&);

    Q_SIGNAL void modeChanged(Enum::ThemeMode);
    Q_SIGNAL void schemeChanged();
    Q_SIGNAL void paletteTypeChanged();
    Q_SIGNAL void accentColorChanged(QColor);
    Q_SIGNAL void useSysColorSMChanged();
    Q_SIGNAL void useSysAccentColorChanged();

private:
    void genSchemeImpl(Enum::ThemeMode);

    QColor                                  m_accent_color;
    Enum::ThemeMode                         m_mode;
    Enum::ThemeMode                         m_last_mode;
    Enum::PaletteType                       m_scheme_type;
    MdScheme                                m_scheme;
    std::map<QColor, QColor, QColorCompare> m_on_map;
    bool                                    m_use_sys_color_scheme;
    bool                                    m_use_sys_accent_color;
};

void sysNotifyInit(MdColorMgr&);
auto sysColorScheme() -> Enum::ThemeMode;
auto sysAccentColor() -> QColor;

} // namespace qml_material
