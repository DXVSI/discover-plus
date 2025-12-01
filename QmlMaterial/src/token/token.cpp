#include "qml_material/token/token.hpp"

namespace qml_material::token
{
Token::Token(QObject* parent)
    : QObject(parent),
      m_typescale(new TypeScale(this)),
      m_icon(create_icon_token(this)),
      m_flick(new Flick(this)) {}
Token::~Token() {}

auto Token::version() const -> QString { return QLatin1String(QM_VERSION); }
auto Token::icon_font_url() const -> QString { return QLatin1String(QM_ICON_FONT_URL); }
auto Token::icon_fill_0_font_url() const -> QString { return QLatin1String(QM_ICON_FILL_0_FONT_URL); }
auto Token::icon_fill_1_font_url() const -> QString { return QLatin1String(QM_ICON_FILL_1_FONT_URL); }

auto Token::typescale() const -> TypeScale* { return m_typescale; }
auto Token::icon() const -> IconToken* { return m_icon; }
auto Token::flick() const -> Flick* { return m_flick; }
auto Token::elevation() const -> const Elevation& { return m_elevation; }
auto Token::state() const -> const State& { return m_state; }
auto Token::shape() const -> const Shape& { return m_shape; }
auto Token::window_class() const -> const WindowClass& { return m_win_class; }
auto Token::duration() const -> const Duration& { return m_duration; }
auto Token::easing() const -> const Easing& { return m_easing; }
auto Token::cal_curve_scale(double dpr) const -> double { return dpr >= 2.0 ? 1.0 : 4.0; }

auto Token::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }

Flick::Flick(QObject* parent)
    : QObject(parent),
      m_press_delay(100),
      m_flick_deceleration(5000),
      m_maximum_flickVelocity(2500) {
    const char* whell_flick_deceleration = "QT_QUICK_FLICKABLE_WHEEL_DECELERATION";
    if (qgetenv(whell_flick_deceleration).size() == 0) {
        qputenv(whell_flick_deceleration, "5000");
    }
}
Flick::~Flick() {}
auto Flick::pressDelay() const -> qint32 { return m_press_delay; }
auto Flick::flickDeceleration() const -> double { return m_flick_deceleration; }
auto Flick::maximumFlickVelocity() const -> double { return m_maximum_flickVelocity; }

} // namespace qml_material::token

#include <qml_material/token/moc_token.cpp>