#pragma once

#include <array>
#include <QtCore/QEasingCurve>
#include <QtGui/QFont>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickWindow>

#include "qml_material/core.hpp"
#include "qml_material/token/type_scale.hpp"
#include "qml_material/anim/interpolator.hpp"
#include "qml_material/enum.hpp"
Q_MOC_INCLUDE("qml_material/token/icon.hpp")

namespace qml_material::token
{

class IconToken;
auto create_icon_token(QObject* parent) -> IconToken*;

/**
 * @brief Represents a window class category with size constraints
 *
 * Defines a window size range with minimum and maximum width values,
 * and associates it with a specific window class type.
 */
struct WindowClassItem {
    Q_GADGET
    QML_ANONYMOUS

    /// Minimum width of the window class range
    Q_PROPERTY(qint32 min_width MEMBER min_width CONSTANT FINAL)
    /// Maximum width of the window class range
    Q_PROPERTY(qint32 max_width MEMBER max_width CONSTANT FINAL)
    /// Type classification of the window size (compact, medium, expanded etc.)
    Q_PROPERTY(qml_material::Enum::WindowClassType type MEMBER type CONSTANT FINAL)

public:
    Q_INVOKABLE bool      contains(i32 w) const { return w >= min_width && w < max_width; }
    i32                   min_width { 0 };
    i32                   max_width { std::numeric_limits<i32>::max() };
    Enum::WindowClassType type { Enum::WindowClassType::WindowClassCompact };
};

/**
 * @brief Window classification system based on width ranges
 *
 * Provides different window class categories (compact, medium, expanded, etc.)
 * and utilities to determine the appropriate class based on window width.
 */
struct WindowClass {
    Q_GADGET
    QML_ANONYMOUS

    /// Window class for compact layouts (0-600px)
    Q_PROPERTY(qml_material::token::WindowClassItem compact MEMBER compact CONSTANT FINAL)
    /// Window class for medium layouts (600-840px)
    Q_PROPERTY(qml_material::token::WindowClassItem medium MEMBER medium CONSTANT FINAL)
    /// Window class for expanded layouts (840-1200px)
    Q_PROPERTY(qml_material::token::WindowClassItem expanded MEMBER expanded CONSTANT FINAL)
    /// Window class for large layouts (1200-1600px)
    Q_PROPERTY(qml_material::token::WindowClassItem large MEMBER large CONSTANT FINAL)
    /// Window class for extra large layouts (1600px+)
    Q_PROPERTY(qml_material::token::WindowClassItem extra_large MEMBER extra_large CONSTANT FINAL)

public:
    using Type = Enum::WindowClassType;

    Q_INVOKABLE WindowClassItem select(i32 w) const {
        std::array wcs { &compact, &medium, &expanded, &large, &extra_large };
        for (auto& el : wcs) {
            if (el->contains(w)) return *el;
        }
        return compact;
    }
    Q_INVOKABLE Enum::WindowClassType select_type(i32 w) const {
        std::array wcs { &compact, &medium, &expanded, &large, &extra_large };
        for (auto& el : wcs) {
            if (el->contains(w)) return el->type;
        }
        return compact.type;
    }
    WindowClassItem compact { 0, 600, Type::WindowClassCompact };
    WindowClassItem medium { 600, 840, Type::WindowClassMedium };
    WindowClassItem expanded { 840, 1200, Type::WindowClassExpanded };
    WindowClassItem large { 1200, 1600, Type::WindowClassLarge };
    WindowClassItem extra_large { 1600,
                                  std::numeric_limits<i32>::max(),
                                  Type::WindowClassExtraLarge };
};

/**
 * @brief Defines elevation levels for Material Design shadows
 *
 * Provides different elevation values (0-5) used to create
 * depth and hierarchy through shadow effects.
 */
struct Elevation {
    Q_GADGET
    QML_ANONYMOUS

    /// No elevation (0dp)
    Q_PROPERTY(qint32 level0 MEMBER level0 CONSTANT FINAL)
    /// Low elevation for subtle depth (1dp)
    Q_PROPERTY(qint32 level1 MEMBER level1 CONSTANT FINAL)
    /// Slight elevation for cards and raised buttons (3dp)
    Q_PROPERTY(qint32 level2 MEMBER level2 CONSTANT FINAL)
    /// Medium elevation for navigation drawers (6dp)
    Q_PROPERTY(qint32 level3 MEMBER level3 CONSTANT FINAL)
    /// High elevation for dialogs (8dp)
    Q_PROPERTY(qint32 level4 MEMBER level4 CONSTANT FINAL)
    /// Maximum elevation for modal surfaces (12dp)
    Q_PROPERTY(qint32 level5 MEMBER level5 CONSTANT FINAL)
public:
    i32 level0 { 0 };
    i32 level1 { 1 };
    i32 level2 { 3 };
    i32 level3 { 6 };
    i32 level4 { 8 };
    i32 level5 { 12 };
};

/**
 * @brief Motion easing
 *
 */
struct Easing {
    Q_GADGET
    QML_ANONYMOUS
    /// Animation curve for emphasized
    Q_PROPERTY(QEasingCurve emphasized READ emphasized CONSTANT FINAL)
    /// Animation curve for emphasized acceleration
    Q_PROPERTY(QEasingCurve emphasized_accelerate READ emphasized_accelerate CONSTANT FINAL)
    /// Animation curve for emphasized deceleration
    Q_PROPERTY(QEasingCurve emphasized_decelerate READ emphasized_decelerate CONSTANT FINAL)
    /// Standard animation curve for balanced motion
    Q_PROPERTY(QEasingCurve standard READ standard CONSTANT FINAL)
    /// Animation curve for standard acceleration
    Q_PROPERTY(QEasingCurve standard_accelerate READ standard_accelerate CONSTANT FINAL)
    /// Animation curve for standard deceleration
    Q_PROPERTY(QEasingCurve standard_decelerate READ standard_decelerate CONSTANT FINAL)
    /// Legacy animation curve (backward compatibility)
    Q_PROPERTY(QEasingCurve legacy READ legacy CONSTANT FINAL)
    /// Legacy acceleration curve
    Q_PROPERTY(QEasingCurve legacy_accelerate READ legacy_accelerate CONSTANT FINAL)
    /// Legacy deceleration curve
    Q_PROPERTY(QEasingCurve legacy_decelerate READ legacy_decelerate CONSTANT FINAL)
    /// Linear animation curve (constant speed)
    Q_PROPERTY(QEasingCurve linear READ linear CONSTANT FINAL)

public:
    auto emphasized() const noexcept -> QEasingCurve { return anim::emphasized(); }
    auto emphasized_accelerate() const noexcept -> QEasingCurve {
        return anim::emphasized_accelerate();
    }
    auto emphasized_decelerate() const noexcept -> QEasingCurve {
        return anim::emphasized_decelerate();
    }
    auto standard() const noexcept -> QEasingCurve { return anim::standard(); }
    auto standard_accelerate() const noexcept -> QEasingCurve {
        return anim::standard_accelerate();
    }
    auto standard_decelerate() const noexcept -> QEasingCurve {
        return anim::standard_decelerate();
    }
    auto legacy() const noexcept -> QEasingCurve { return anim::legacy(); }
    auto legacy_accelerate() const noexcept -> QEasingCurve { return anim::legacy_accelerate(); }
    auto legacy_decelerate() const noexcept -> QEasingCurve { return anim::legacy_decelerate(); }
    auto linear() const noexcept -> QEasingCurve { return anim::linear(); }
};

/**
 * @brief Motion duration
 *
 */
struct Duration {
    Q_GADGET
    QML_ANONYMOUS
    /// Extra short duration (50ms)
    Q_PROPERTY(qreal short1 MEMBER short1 CONSTANT FINAL)
    /// Short duration (100ms)
    Q_PROPERTY(qreal short2 MEMBER short2 CONSTANT FINAL)
    /// Short-medium duration (150ms)
    Q_PROPERTY(qreal short3 MEMBER short3 CONSTANT FINAL)
    /// Medium-short duration (200ms)
    Q_PROPERTY(qreal short4 MEMBER short4 CONSTANT FINAL)
    /// Medium duration (250ms)
    Q_PROPERTY(qreal medium1 MEMBER medium1 CONSTANT FINAL)
    /// Medium-standard duration (300ms)
    Q_PROPERTY(qreal medium2 MEMBER medium2 CONSTANT FINAL)
    /// Medium-long duration (350ms)
    Q_PROPERTY(qreal medium3 MEMBER medium3 CONSTANT FINAL)
    /// Extended medium duration (400ms)
    Q_PROPERTY(qreal medium4 MEMBER medium4 CONSTANT FINAL)
    /// Long duration (450ms)
    Q_PROPERTY(qreal long1 MEMBER long1 CONSTANT FINAL)
    /// Extended long duration (500ms)
    Q_PROPERTY(qreal long2 MEMBER long2 CONSTANT FINAL)
    /// Very long duration (550ms)
    Q_PROPERTY(qreal long3 MEMBER long3 CONSTANT FINAL)
    /// Maximum long duration (600ms)
    Q_PROPERTY(qreal long4 MEMBER long4 CONSTANT FINAL)
    /// Extra long duration (700ms)
    Q_PROPERTY(qreal extra_long1 MEMBER extra_long1 CONSTANT FINAL)
    /// Extended extra long duration (800ms)
    Q_PROPERTY(qreal extra_long2 MEMBER extra_long2 CONSTANT FINAL)
    /// Very extra long duration (900ms)
    Q_PROPERTY(qreal extra_long3 MEMBER extra_long3 CONSTANT FINAL)
    /// Maximum extra long duration (1000ms)
    Q_PROPERTY(qreal extra_long4 MEMBER extra_long4 CONSTANT FINAL)

public:
    qreal short1 { 50 };
    qreal short2 { 100 };
    qreal short3 { 150 };
    qreal short4 { 200 };
    qreal medium1 { 250 };
    qreal medium2 { 300 };
    qreal medium3 { 350 };
    qreal medium4 { 400 };
    qreal long1 { 450 };
    qreal long2 { 500 };
    qreal long3 { 550 };
    qreal long4 { 600 };
    qreal extra_long1 { 700 };
    qreal extra_long2 { 800 };
    qreal extra_long3 { 900 };
    qreal extra_long4 { 1000 };
};

/**
 * @brief Defines corner radius values for shape styling
 *
 * Contains predefined corner radius values from none (0) to full (100),
 * used for consistent shape styling across the UI.
 */
struct ShapeCorner {
    Q_GADGET
    QML_ANONYMOUS
    /// No corner radius (0dp)
    Q_PROPERTY(qint32 none MEMBER none CONSTANT FINAL)
    /// Extra small corner radius (4dp)
    Q_PROPERTY(qint32 extra_small MEMBER extra_small CONSTANT FINAL)
    /// Small corner radius (8dp)
    Q_PROPERTY(qint32 small MEMBER small CONSTANT FINAL)
    /// Medium corner radius (12dp)
    Q_PROPERTY(qint32 medium MEMBER medium CONSTANT FINAL)
    /// Large corner radius (16dp)
    Q_PROPERTY(qint32 large MEMBER large CONSTANT FINAL)
    /// Extra large corner radius (28dp)
    Q_PROPERTY(qint32 extra_large MEMBER extra_large CONSTANT FINAL)
    /// Full circular corner radius (100dp)
    Q_PROPERTY(qint32 full MEMBER full CONSTANT FINAL)
public:
    /// dp
    i32 none { 0 };
    i32 extra_small { 4 };
    i32 small { 8 };
    i32 medium { 12 };
    i32 large { 16 };
    i32 extra_large { 28 };
    i32 full { 100 };
};

/**
 * @brief Container for shape-related styling properties
 *
 * Currently encapsulates corner radius settings for UI elements.
 */
struct Shape {
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::token::ShapeCorner corner MEMBER corner CONSTANT FINAL)
public:
    ShapeCorner corner;
};

/**
 * @brief Defines opacity values for different UI states
 *
 * Contains opacity settings used when elements are in specific
 * interactive states.
 */
struct StateItem {
    Q_GADGET
    QML_ANONYMOUS
    /// Opacity value for the state layer when element is in this state
    Q_PROPERTY(double state_layer_opacity MEMBER state_layer_opacity CONSTANT FINAL)
public:
    double state_layer_opacity;
};

/**
 * @brief Manages UI element state appearances
 *
 * Defines visual feedback for different interaction states
 * like hover, pressed, and focus.
 */
struct State {
    Q_GADGET
    QML_ANONYMOUS
    /// State appearance when element is hovered
    Q_PROPERTY(qml_material::token::StateItem hover MEMBER hover CONSTANT FINAL)
    /// State appearance when element is pressed
    Q_PROPERTY(qml_material::token::StateItem pressed MEMBER pressed CONSTANT FINAL)
    /// State appearance when element is focused
    Q_PROPERTY(qml_material::token::StateItem focus MEMBER focus CONSTANT FINAL)

public:
    StateItem hover { 0.08 };
    StateItem pressed { 0.1 };
    StateItem focus { 0.1 };
};

/**
 * @brief Handles flicking behavior and physics parameters
 *
 * Controls the flicking interaction parameters including press delay,
 * deceleration rate, and maximum velocity for scrollable content.
 */
class Flick : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    /// Delay in milliseconds before a press event is recognized
    Q_PROPERTY(qint32 pressDelay READ pressDelay NOTIFY pressDelayChanged FINAL)
    /// Deceleration rate for flicking motion in pixels/second²
    Q_PROPERTY(
        double flickDeceleration READ flickDeceleration NOTIFY flickDecelerationChanged FINAL)
    /// Maximum velocity allowed for flicking in pixels/second
    Q_PROPERTY(double maximumFlickVelocity READ maximumFlickVelocity NOTIFY
                   maximumFlickVelocityChanged FINAL)
public:
    Flick(QObject* parent = nullptr);
    ~Flick();
    auto pressDelay() const -> qint32;
    auto flickDeceleration() const -> double;
    auto maximumFlickVelocity() const -> double;

    Q_SIGNAL void pressDelayChanged();
    Q_SIGNAL void flickDecelerationChanged();
    Q_SIGNAL void maximumFlickVelocityChanged();

private:
    qint32 m_press_delay;
    double m_flick_deceleration;
    double m_maximum_flickVelocity;
};

/**
 * @brief Main token container for Material Design system
 *
 * Centralizes access to various Material Design tokens including typography,
 * icons, animations, elevations, states, shapes, and window classifications.
 */
class Token : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_NAMED_ELEMENT(TokenImpl)

    Q_PROPERTY(QString version READ version CONSTANT FINAL)
    Q_PROPERTY(QString iconFontUrl READ icon_font_url CONSTANT FINAL)
    Q_PROPERTY(QString iconFill0FontUrl READ icon_fill_0_font_url CONSTANT FINAL)
    Q_PROPERTY(QString iconFill1FontUrl READ icon_fill_1_font_url CONSTANT FINAL)

    /// Typography scale settings
    Q_PROPERTY(qml_material::token::TypeScale* typescale READ typescale CONSTANT FINAL)
    /// Icon token settings and management
    Q_PROPERTY(qml_material::token::IconToken* icon READ icon CONSTANT FINAL)
    /// Flick interaction settings
    Q_PROPERTY(qml_material::token::Flick* flick READ flick CONSTANT FINAL)
    /// Elevation levels for shadows
    Q_PROPERTY(qml_material::token::Elevation elevation READ elevation CONSTANT FINAL)
    /// Motion durations
    Q_PROPERTY(qml_material::token::Duration duration READ duration CONSTANT FINAL)
    /// Motion Easing
    Q_PROPERTY(qml_material::token::Easing easing READ easing CONSTANT FINAL)
    /// State appearance settings
    Q_PROPERTY(qml_material::token::State state READ state CONSTANT FINAL)
    /// Shape styling settings
    Q_PROPERTY(qml_material::token::Shape shape READ shape CONSTANT FINAL)
    /// Window classification settings
    Q_PROPERTY(qml_material::token::WindowClass window_class READ window_class CONSTANT FINAL)
public:
    Token(QObject* = nullptr);
    ~Token();

    auto version() const -> QString;
    auto icon_font_url() const -> QString;
    auto icon_fill_0_font_url() const -> QString;
    auto icon_fill_1_font_url() const -> QString;

    auto typescale() const -> TypeScale*;
    auto icon() const -> IconToken*;
    auto flick() const -> Flick*;
    auto elevation() const -> const Elevation&;
    auto state() const -> const State&;
    auto shape() const -> const Shape&;
    auto window_class() const -> const WindowClass&;
    auto duration() const -> const Duration&;
    auto easing() const -> const Easing&;

    auto datas() -> QQmlListProperty<QObject>;

    Q_INVOKABLE double cal_curve_scale(double dpr) const;

private:
    TypeScale*  m_typescale;
    IconToken*  m_icon;
    Flick*      m_flick;
    Duration    m_duration;
    Easing      m_easing;
    Elevation   m_elevation;
    State       m_state;
    Shape       m_shape;
    WindowClass m_win_class;

    QList<QObject*> m_datas;
};
} // namespace qml_material::token