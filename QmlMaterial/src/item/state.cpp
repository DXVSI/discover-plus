#include "qml_material/item/state.hpp"
#include <QQmlContext>
#include <QQmlProperty>
#include <QVariant>
#include <QtQuick/private/qquickstategroup_p.h>
#include "qml_material/theme.hpp"

namespace qml_material
{
State::State(QQuickItem* parent)
    : QObject(parent),
      m_target(nullptr),
      m_enabled(true),
      m_ctx(nullptr),
      m_explicit_ctx(false),
      m_elevation(0),
      m_text_color(),
      m_outline_color(),
      m_background_color(),
      m_support_text_color(),
      m_state_layer_color("transparent"),
      m_state_layer_opacity(0),
      m_content_opacity(1.0),
      m_background_opacity(1.0),
      m_component_complete(true),
      m_state_group(nullptr) {
    connect(this, &State::targetChanged, this, &State::updateCtx, Qt::DirectConnection);
}
State::~State() {}

auto State::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }

QQuickStateGroup* State::_states() {
    if (! m_state_group) {
        m_state_group = new QQuickStateGroup;
        if (! m_component_complete) m_state_group->classBegin();

        qmlobject_connect(m_state_group,
                          QQuickStateGroup,
                          SIGNAL(stateChanged(QString)),
                          this,
                          State,
                          SIGNAL(stateChanged(QString)));
    }
    return m_state_group;
}
QQmlListProperty<QQuickState> State::states() { return _states()->statesProperty(); }

QQmlListProperty<QQuickTransition> State::transitions() { return _states()->transitionsProperty(); }

QString State::state() const {
    if (! m_state_group)
        return QString();
    else
        return m_state_group->state();
}
void State::set_state(const QString& state) { _states()->setState(state); }

void State::classBegin() {
    m_component_complete = false;
    if (m_state_group) m_state_group->classBegin();
}
void State::componentComplete() {
    m_component_complete = true;
    if (m_state_group) m_state_group->componentComplete();
}

auto State::ctx() const -> Theme* { return m_ctx; }
void State::set_ctx(Theme* v) {
    if (m_ctx != v) {
        m_ctx          = v;
        m_explicit_ctx = true;
        ctxChanged();
    }
}

void State::updateCtx() {
    if (m_target != nullptr) {
        if (auto a = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(m_target, true))) {
            if (m_ctx != a && ! m_explicit_ctx) {
                m_ctx = a;
                ctxChanged();
            }
        }
    }
}

auto State::enabled() const noexcept -> bool { return m_enabled; }
void State::set_enabled(bool v) {
    if (m_enabled != v) {
        m_enabled = v;
        enabledChanged();
    }
}

auto State::target() const noexcept -> QObject* { return m_target; }
void State::set_target(QObject* item) {
    if (m_target != item) {
        m_target = item;
        targetChanged();
    }
}

#define X(type, name, sig)                                         \
    auto State::name() const noexcept -> type { return m_##name; } \
    void State::set_##name(type val) {                             \
        if (ycore::cmp_set(m_##name, val)) {                       \
            sig();                                                 \
        }                                                          \
    }

X(qint32, elevation, elevationChanged)
X(QColor, text_color, textColorChanged)
X(QColor, outline_color, outlineColorChanged)
X(QColor, background_color, backgroundColorChanged)
X(QColor, support_text_color, supportTextColorChanged)
X(QColor, state_layer_color, stateLayerColorChanged)
X(double, state_layer_opacity, stateLayerOpacityChanged)
X(double, content_opacity, contentOpacityChanged)
X(double, background_opacity, backgroundOpacityChanged)
#undef X

} // namespace qml_material

#include <qml_material/item/moc_state.cpp>