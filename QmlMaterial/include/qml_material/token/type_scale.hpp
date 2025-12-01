#pragma once

#include <QQmlEngine>
#include <QFont>
#include "qml_material/core.hpp"

namespace qml_material::token
{
//  Font.Thin	0
//  Font.ExtraLight	12
//  Font.Light	25
//  Font.Normal	50
//  Font.Medium	57
//  Font.DemiBold	63
//  Font.Bold	75
//  Font.ExtraBold	81
//  Font.Black	87

// Value 	Common weight name
// 100 	Thin (Hairline)
// 200 	Extra Light (Ultra Light)
// 300 	Light
// 400 	Normal (Regular)
// 500 	Medium
// 600 	Semi Bold (Demi Bold)
// 700 	Bold
// 800 	Extra Bold (Ultra Bold)
// 900 	Black (Heavy)
// 950 	Extra Black (Ultra Black)

/**
 * @brief TypeScaleItem
 *
 */
struct TypeScaleItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(typescale)

    /** @brief Font size in pixels */
    Q_PROPERTY(qint32 size MEMBER size FINAL)
    /** @brief Line height in pixels */
    Q_PROPERTY(qint32 line_height MEMBER line_height FINAL)
    /** @brief Font weight for normal text */
    Q_PROPERTY(QFont::Weight weight MEMBER weight FINAL)
    /** @brief Font weight for prominent (emphasized) text */
    Q_PROPERTY(QFont::Weight weight_prominent MEMBER weight_prominent FINAL)
    /** @brief Letter spacing (tracking) adjustment */
    Q_PROPERTY(qreal tracking MEMBER tracking FINAL)
public:
    Q_INVOKABLE TypeScaleItem fresh() const { return *this; }

    i32           size { 14 };
    i32           line_height { 24 };
    QFont::Weight weight { QFont::Weight::Normal };
    QFont::Weight weight_prominent { QFont::Weight::Normal };
    qreal         tracking { 0 };
};

/**
 * @brief TypeScale
 *
 */
class TypeScale : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

#define X(NAME, ...)                                                             \
    Q_PROPERTY(qml_material::token::TypeScaleItem NAME READ NAME CONSTANT FINAL) \
public:                                                                          \
    const TypeScaleItem& NAME() const noexcept { return m_##NAME; }              \
                                                                                 \
private:                                                                         \
    TypeScaleItem m_##NAME { __VA_ARGS__ };

    // clang-format off
    /** @brief Used for the largest text on the screen, such as app name on splash screen */
    X(display_large  , 57, 64, QFont::Normal, QFont::Normal, -0.25)
    /** @brief Used for medium-sized display text, secondary splash screen text */
    X(display_medium , 45, 52, QFont::Normal, QFont::Normal, 0.0  )
    /** @brief Used for smaller display text */
    X(display_small  , 36, 44, QFont::Normal, QFont::Normal, 0.0  )
    /** @brief Primary heading text, such as screen titles */
    X(headline_large , 32, 40, QFont::Medium, QFont::Medium, 0.0  )
    /** @brief Secondary heading text */
    X(headline_medium, 28, 36, QFont::Medium, QFont::Medium, 0.0  )
    /** @brief Smallest heading text */
    X(headline_small , 24, 32, QFont::Medium, QFont::Medium, 0.0  )
    /** @brief Used for primary text in dialogs, cards */
    X(title_large    , 22, 28, QFont::Normal, QFont::Normal, 0.0  )
    /** @brief Secondary text in dialogs, cards */
    X(title_medium   , 16, 24, QFont::Medium, QFont::Medium, 0.15 )
    /** @brief Supporting text in components */
    X(title_small    , 14, 20, QFont::Medium, QFont::Medium, 0.1  )
    /** @brief Used for primary body text */
    X(body_large     , 16, 24, QFont::Normal, QFont::Normal, 0.5  )
    /** @brief Used for medium-sized body text */
    X(body_medium    , 14, 20, QFont::Normal, QFont::Normal, 0.25 )
    /** @brief Used for small body text */
    X(body_small     , 12, 16, QFont::Normal, QFont::Normal, 0.4  )
    /** @brief Large text labels for important interface elements */
    X(label_large    , 14, 20, QFont::Medium, QFont::Bold  , 0.1  )
    /** @brief Medium-sized text labels */
    X(label_medium   , 12, 16, QFont::Medium, QFont::Bold  , 0.5  )
    /** @brief Smallest text labels for interface elements */
    X(label_small    , 11, 16, QFont::Medium, QFont::Bold  , 0.5  )
    // clang-format on

#undef X
};

} // namespace qml_material::token
