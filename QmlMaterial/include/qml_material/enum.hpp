#pragma once
#include <QObject>
#include <QQmlEngine>
#include "qml_material/export.hpp"

namespace qml_material
{

/**
 * @brief Enum
 *
 */
class QML_MATERIAL_API Enum : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

    enum class ThemeMode
    {
        Light = 0, ///< Light
        Dark       ///< Dark
    };
    Q_ENUM(ThemeMode)

    enum class IconLabelStyle
    {
        IconAndText = 0, ///< Icon And Test
        IconOnly,        ///< Icon Only
        TextOnly         ///< Text Only
    };
    Q_ENUM(IconLabelStyle)

    enum class ButtonType
    {
        BtElevated = 0, ///< Elevated
        BtFilled,       ///< Filled
        BtFilledTonal,  ///< Filled Tonal
        BtOutlined,     ///< Outlined
        BtText          ///< Text
    };
    Q_ENUM(ButtonType)

    enum class IconButtonType
    {
        IBtFilled = 0,  ///< Filled
        IBtFilledTonal, ///< Filled Tonal
        IBtOutlined,    ///< Outlined
        IBtStandard     ///< Standard
    };
    Q_ENUM(IconButtonType)

    enum class FABType
    {
        FABSmall = 0, ///< Small floating action button
        FABNormal,    ///< Normal sized floating action button
        FABLarge      ///< Large floating action button
    };
    Q_ENUM(FABType)

    enum class FABColor
    {
        FABColorPrimary = 0, ///< Primary color
        FABColorSurfaec,     ///< Surface color
        FABColorSecondary,   ///< Secondary color
        FABColorTertiary     ///< Tertiary color
    };
    Q_ENUM(FABColor)

    enum class CardType
    {
        CardElevated = 0, ///< Elevated card style
        CardFilled,       ///< Filled card style
        CardOutlined      ///< Outlined card style
    };
    Q_ENUM(CardType)

    enum class TextFieldType
    {
        TextFieldFilled = 0, ///< Filled text field style
        TextFieldOutlined    ///< Outlined text field style
    };
    Q_ENUM(TextFieldType)

    enum class ListItemHeightMode
    {
        ListItemOneLine = 0, ///< Single line list item
        ListItemTwoLine,     ///< Two line list item
        ListItemThreeLine    ///< Three line list item
    };
    Q_ENUM(ListItemHeightMode)

    enum class TabType
    {
        PrimaryTab,  ///< Primary tab style
        SecondaryTab ///< Secondary tab style
    };
    Q_ENUM(TabType)

    enum class AppBarType
    {
        AppBarCenterAligned, ///< Center aligned app bar
        AppBarSmall,         ///< Small app bar
        AppBarMedium,        ///< Medium app bar
        AppBarLarge          ///< Large app bar
    };
    Q_ENUM(AppBarType)

    enum class ToastFlag
    {
        TFCloseable = 1,      ///< Toast can be closed
        TFSave      = 1 << 1, ///< Toast can be saved
    };
    Q_ENUM(ToastFlag)
    Q_DECLARE_FLAGS(ToastFlags, ToastFlag)

    enum class WindowClassType
    {
        WindowClassCompact,    ///< Compact window size
        WindowClassMedium,     ///< Medium window size
        WindowClassExpanded,   ///< Expanded window size
        WindowClassLarge,      ///< Large window size
        WindowClassExtraLarge, ///< Extra large window size
    };
    Q_ENUM(WindowClassType)

    enum class PaletteType
    {
        PaletteNeutral = 0,
        PaletteRainbow,
        PaletteTonalSpot,
        PaletteMonochrome,
        PaletteVibrant,
        PaletteContent,
        PaletteFidelity,
        PaletteExpressive,
        PaletteFruitSalad,
    };
    Q_ENUM(PaletteType)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Enum::ToastFlags);
} // namespace qml_material