#pragma once

#include <QtCore/QObject>
#include <QtGui/QColor>

#include "qml_material/enum.hpp"

#include "cpp/palettes/tones.h"

namespace qml_material
{
struct MdScheme {
    QRgb primary;
    QRgb on_primary;
    QRgb primary_container;
    QRgb on_primary_container;
    QRgb secondary;
    QRgb on_secondary;
    QRgb secondary_container;
    QRgb on_secondary_container;
    QRgb tertiary;
    QRgb on_tertiary;
    QRgb tertiary_container;
    QRgb on_tertiary_container;
    QRgb error;
    QRgb on_error;
    QRgb error_container;
    QRgb on_error_container;
    QRgb background;
    QRgb on_background;
    QRgb surface;
    QRgb on_surface;
    QRgb surface_variant;
    QRgb on_surface_variant;
    QRgb outline;
    QRgb outline_variant;
    QRgb shadow;
    QRgb scrim;
    QRgb inverse_surface;
    QRgb inverse_on_surface;
    QRgb inverse_primary;

    // surface
    QRgb surface_1;
    QRgb surface_2;
    QRgb surface_3;
    QRgb surface_4;
    QRgb surface_5;

    // surface v2
    QRgb surface_dim;
    QRgb surface_bright;
    QRgb surface_container;
    QRgb surface_container_low;
    QRgb surface_container_lowest;
    QRgb surface_container_high;
    QRgb surface_container_highest;

    material_color_utilities::TonalPalette neutral_palette { 0 };
};

auto material_light_color_scheme(QRgb, Enum::PaletteType) -> MdScheme;
auto material_dark_color_scheme(QRgb, Enum::PaletteType) -> MdScheme;

auto material_blend_hcthue(const QRgb design_color, const QRgb key_color, const double mount) -> QRgb;
auto color_from_image(const QImage&) -> QRgb;
} // namespace qml_material
