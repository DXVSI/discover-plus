#include "qml_material/helper.hpp"

#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fidelity.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"

#include "cpp/quantize/celebi.h"
#include "cpp/score/score.h"

#include "cpp/blend/blend.h"

#include "qml_material/core.hpp"

#include "cpp/palettes/core.h"
#include <QtGui/QImage>

using MdScheme = qml_material::MdScheme;
namespace md   = material_color_utilities;
using Hct      = md::Hct;

namespace
{

QRgb blend(QRgb a, QRgb b, double t) {
    double dt = 1.0 - t;
    return qRgba(qRed(a) * dt + qRed(b) * t,
                 qGreen(a) * dt + qGreen(b) * t,
                 qBlue(a) * dt + qBlue(b) * t,
                 qAlpha(a));
}
void convert_from(MdScheme& out, const md::DynamicScheme& in) {
    out.primary                = in.GetPrimary();
    out.on_primary             = in.GetOnPrimary();
    out.primary_container      = in.GetPrimaryContainer();
    out.on_primary_container   = in.GetOnPrimaryContainer();
    out.secondary              = in.GetSecondary();
    out.on_secondary           = in.GetOnSecondary();
    out.secondary_container    = in.GetSecondaryContainer();
    out.on_secondary_container = in.GetOnSecondaryContainer();
    out.tertiary               = in.GetTertiary();
    out.on_tertiary            = in.GetOnTertiary();
    out.tertiary_container     = in.GetTertiaryContainer();
    out.on_tertiary_container  = in.GetOnTertiaryContainer();
    out.error                  = in.GetError();
    out.on_error               = in.GetOnError();
    out.error_container        = in.GetErrorContainer();
    out.on_error_container     = in.GetOnErrorContainer();
    out.background             = in.GetBackground();
    out.on_background          = in.GetOnBackground();
    out.surface                = in.GetSurface();
    out.on_surface             = in.GetOnSurface();
    out.surface_variant        = in.GetSurfaceVariant();
    out.on_surface_variant     = in.GetOnSurfaceVariant();
    out.outline                = in.GetOutline();
    out.outline_variant        = in.GetOutlineVariant();
    out.shadow                 = in.GetShadow();
    out.scrim                  = in.GetScrim();
    out.inverse_surface        = in.GetInverseSurface();
    out.inverse_on_surface     = in.GetInverseOnSurface();
    out.inverse_primary        = in.GetInversePrimary();

    out.surface_dim               = in.GetSurfaceDim();
    out.surface_bright            = in.GetSurfaceBright();
    out.surface_container         = in.GetSurfaceContainer();
    out.surface_container_low     = in.GetSurfaceContainerLow();
    out.surface_container_lowest  = in.GetSurfaceContainerLowest();
    out.surface_container_high    = in.GetSurfaceContainerHigh();
    out.surface_container_highest = in.GetSurfaceContainerHighest();

    out.surface_1 = blend(out.surface, out.primary, 0.05);
    out.surface_2 = blend(out.surface, out.primary, 0.08);
    out.surface_3 = blend(out.surface, out.primary, 0.11);
    out.surface_4 = blend(out.surface, out.primary, 0.12);
    out.surface_5 = blend(out.surface, out.primary, 0.14);

    out.neutral_palette = in.neutral_palette;
}

auto genScheme(qml_material::Enum::PaletteType t, Hct hct, bool is_dark, double ct_level) {
    MdScheme out;
    using ET = qml_material::Enum::PaletteType;
    switch (t) {
    case ET::PaletteTonalSpot: {
        convert_from(out, md::SchemeTonalSpot(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteNeutral: {
        convert_from(out, md::SchemeNeutral(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteContent: {
        convert_from(out, md::SchemeContent(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteExpressive: {
        convert_from(out, md::SchemeExpressive(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteFidelity: {
        convert_from(out, md::SchemeFidelity(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteFruitSalad: {
        convert_from(out, md::SchemeFruitSalad(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteMonochrome: {
        convert_from(out, md::SchemeMonochrome(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteRainbow: {
        convert_from(out, md::SchemeRainbow(hct, is_dark, ct_level));
        break;
    }
    case ET::PaletteVibrant: {
        convert_from(out, md::SchemeVibrant(hct, is_dark, ct_level));
        break;
    }
    default: {
        convert_from(out, md::SchemeTonalSpot(hct, is_dark, ct_level));
        break;
    }
    }
    return out;
}
} // namespace

MdScheme qml_material::material_light_color_scheme(QRgb rgb, Enum::PaletteType type) {
    md::Hct hct(rgb);
    return genScheme(type, hct, false, 0.0);
}

MdScheme qml_material::material_dark_color_scheme(QRgb rgb, Enum::PaletteType type) {
    md::Hct hct(rgb);
    return genScheme(type, hct, true, 0.0);
}

QRgb qml_material::material_blend_hcthue(const QRgb design_color, const QRgb key_color,
                                         const double mount) {
    return md::BlendHctHue(design_color, key_color, mount);
}

auto qml_material::color_from_image(const QImage& image) -> QRgb {
    std::vector<QRgb> pixels;
    pixels.reserve(image.width() * image.height());
    for (int y = 0; y < image.height(); ++y) {
        QRgb const* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const auto& rgb = line[x];
            auto        p   = qRgba(qRed(rgb), qGreen(rgb), qBlue(rgb), qAlpha(rgb));
            pixels.push_back(p);
        }
    }
    auto quantize_res = material_color_utilities::QuantizeCelebi(pixels, 12);
    return material_color_utilities::RankedSuggestions(quantize_res.color_to_count).at(0);
}
