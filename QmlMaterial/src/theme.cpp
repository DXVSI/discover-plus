#include "qml_material/theme.hpp"
#include "qml_material/enum.hpp"
#include "qml_material/token/token.hpp"

using namespace qml_material;

namespace
{
using Propagator = QQuickAttachedPropertyPropagator;

template<typename F, typename T>
concept get_prop_cp = requires(F f, Theme* t) {
    { std::invoke(f, t) } -> std::same_as<Theme::AttachProp<T>&>;
};

template<typename F>
void propagate(Theme* self, F&& f) {
    const auto styles = self->attachedChildren();
    for (auto* child : styles) {
        Theme* obj = qobject_cast<Theme*>(child);
        if (obj) {
            f(obj);
        }
    }
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void inherit_attach_prop(Theme* self, F&& get_prop, const T& v) {
    auto& p = std::invoke(get_prop, self);
    if (p.explicited || p.value == v) return;
    p.value = v;
    propagate(self, [&v, &get_prop](Theme* child) {
        inherit_attach_prop(child, get_prop, v);
    });
    std::invoke(p.sig_func, self);
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void set_prop(Theme* self, const T& v, F&& get_prop) {
    auto& p      = std::invoke(std::forward<F>(get_prop), self);
    p.explicited = true;
    if (p.value != v) {
        p.value = v;
        if constexpr (std::is_base_of_v<QObject, std::remove_pointer_t<T>>) {
            if (p.value && *p.value) {
                (*p.value)->setParent(self);
            }
        }
        propagate(self, [&v, &get_prop](Theme* child) {
            inherit_attach_prop(child, get_prop, v);
        });
        std::invoke(p.sig_func, self);
    }
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void reset_prop(Theme* self, F&& get_prop, const T& init_v) {
    auto& p = std::invoke(std::forward<F>(get_prop), self);
    if (! p.explicited) return;
    p.explicited = false;
    inherit_attach_prop(self, get_prop, init_v);
}

struct GlobalTheme {
    ~GlobalTheme() {}
    QColor      textColor;
    QColor      supportTextColor;
    QColor      backgroundColor;
    QColor      stateLayerColor;
    int         elevation { 0 };
    MdColorMgr  color_;
    MdColorMgr* color { &color_ };

    ThemeSize  size_;
    ThemeSize* size { &size_ };

    PageContext  page_;
    PageContext* page { &page_ };
};
Q_GLOBAL_STATIC(GlobalTheme, theGlobalTheme)

} // namespace

Theme::Theme(QObject* parent): QQuickAttachedPropertyPropagator(parent) {
    QQuickAttachedPropertyPropagator::initialize();
}
Theme::~Theme() {}

Theme* Theme::qmlAttachedProperties(QObject* object) { return new Theme(object); }

#define IMPL_ATTACH_PROP(_type_, _name_, _prop_, ...)                                           \
    Theme::AttachProp<_type_>& Theme::get_##_name_() { return _prop_; }                         \
    _type_ Theme::_name_() const { return _prop_.value.value_or(theGlobalTheme->_name_); }      \
    void   Theme::set_##_name_(_type_ v) { set_prop(this, v, &Theme::get_##_name_); }           \
    void   Theme::reset_##_name_() {                                                            \
        Self* obj = qobject_cast<Self*>(attachedParent());                                    \
        reset_prop(this, &Theme::get_##_name_, obj ? obj->_name_() : theGlobalTheme->_name_); \
    }

IMPL_ATTACH_PROP(QColor, textColor, m_textColor)
IMPL_ATTACH_PROP(QColor, backgroundColor, m_backgroundColor)
IMPL_ATTACH_PROP(int, elevation, m_elevation)
IMPL_ATTACH_PROP(MdColorMgr*, color, m_color)
IMPL_ATTACH_PROP(ThemeSize*, size, m_size)
IMPL_ATTACH_PROP(PageContext*, page, m_page)

void Theme::attachedParentChange(QQuickAttachedPropertyPropagator* newParent,
                                 QQuickAttachedPropertyPropagator* oldParent) {
    Propagator::attachedParentChange(newParent, oldParent);
    Theme* attachedParentStyle = qobject_cast<Theme*>(newParent);
    if (attachedParentStyle) {
#define X(Name) inherit_attach_prop(this, &Theme::get_##Name, attachedParentStyle->Name())
        X(textColor);
        X(backgroundColor);
        X(elevation);
        X(color);
        X(size);
        X(page);
#undef X
    }
}

ThemeSize::ThemeSize(QObject* parent)
    : QObject(parent),
      m_window_class((qint32)Enum::WindowClassType::WindowClassMedium),
      m_width(0),
      m_duration(200) {
    m_width_timer.setSingleShot(true);
    connect(this, &ThemeSize::widthChanged, this, [this]() {
        if (m_duration > 0) {
            if (! m_width_timer.isActive()) m_width_timer.start(m_duration);
        } else {
            static auto the_wclass = token::WindowClass {};
            setWindowClass((qint32)the_wclass.select_type(m_width));
        }
    });
    connect(&m_width_timer, &QTimer::timeout, this, [this]() {
        static auto the_wclass = token::WindowClass {};
        setWindowClass((qint32)the_wclass.select_type(m_width));
    });
}
ThemeSize::~ThemeSize() {}

auto ThemeSize::isCompact() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassCompact;
}
auto ThemeSize::isMedium() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassMedium;
}
auto ThemeSize::isExpanded() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassExpanded;
}
auto ThemeSize::isLarge() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassLarge;
}
auto ThemeSize::isExtraLarge() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassExtraLarge;
}

auto ThemeSize::width() const -> qint32 { return m_width; }

void ThemeSize::setWidth(qint32 v) {
    if (m_width != v) {
        m_width = v;
        widthChanged(m_width);
    }
}
auto ThemeSize::duration() const -> qint32 { return m_duration; }

void ThemeSize::setDuration(qint32 v) {
    if (m_duration != v) {
        m_duration = v;
        durationChanged(m_width);
    }
}

auto ThemeSize::windowClass() const -> qint32 { return m_window_class; }
void ThemeSize::setWindowClass(qint32 v) {
    if (v != m_window_class) {
        m_window_class = v;
        windowClassChanged(m_window_class);

        verticalPaddingChanged();
    }
}
auto ThemeSize::verticalPadding() const -> qint32 {
    using WT = Enum::WindowClassType;
    switch ((WT)windowClass()) {
    case WT::WindowClassCompact: return 8;
    default: return 16;
    }
}

#include <qml_material/moc_theme.cpp>