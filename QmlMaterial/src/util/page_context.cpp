#include "qml_material/util/page_context.hpp"
#include "qml_material/enum.hpp"

namespace qml_material
{
PageContext::PageContext(QObject* parent): QObject(parent), m_inherit(nullptr) {}

PageContext::~PageContext() {}

auto PageContext::inherit() const -> PageContext* { return m_inherit; }
void PageContext::setInherit(PageContext* v) {
    if (v != m_inherit) {
        m_inherit = v;
        inheritChanged();
    }
}

#define X(Func, Mem, Def) Mem.value_or(m_inherit != nullptr ? m_inherit->Func() : Def)

auto PageContext::leadingAction() const -> QObject* {
    return X(leadingAction, m_leading_action, nullptr);
}
void PageContext::setLeadingAction(QObject* v) {
    if (v != m_leading_action) {
        m_leading_action = v;
        if (v && v->parent() == nullptr) {
            v->setParent(this);
        }
        leadingActionChanged();
    }
}

auto PageContext::headerType() const -> qint32 {
    return X(headerType, m_header_type, (qint32)Enum::AppBarType::AppBarSmall);
}
void PageContext::setHeaderType(qint32 v) {
    if (v != m_header_type) {
        m_header_type = v;
        headerTypeChanged();
    }
}

auto PageContext::backgroundRadius() const -> qint32 {
    return X(backgroundRadius, m_background_radius, 0);
}
void PageContext::setBackgroundRadius(qint32 v) {
    if (v != m_background_radius) {
        m_background_radius = v;
        backgroundRadiusChanged();
    }
}

auto PageContext::headerBackgroundOpacity() const -> double {
    return X(headerBackgroundOpacity, m_header_background_opacity, 1.0);
}
void PageContext::setHeaderBackgroundOpacity(double v) {
    if (! m_header_background_opacity || ! qFuzzyCompare(m_header_background_opacity.value(), v)) {
        m_header_background_opacity = v;
        headerBackgroundOpacityChanged();
    }
}

auto PageContext::radius() const -> qint32 { return X(radius, m_radius, 0); }
void PageContext::setRadius(qint32 v) {
    if (v != m_radius) {
        m_radius = v;
        radiusChanged();
    }
}

auto PageContext::showHeader() const -> bool { return X(showHeader, m_show_header, false); }
void PageContext::setShowHeader(bool v) {
    if (v != m_show_header) {
        m_show_header = v;
        showHeaderChanged();
    }
}

auto PageContext::showBackground() const -> bool {
    return X(showBackground, m_show_background, false);
}
void PageContext::setShowBackground(bool v) {
    if (v != m_show_background) {
        m_show_background = v;
        showBackgroundChanged();
    }
}
#undef X
} // namespace qml_material

#include <qml_material/util/moc_page_context.cpp>