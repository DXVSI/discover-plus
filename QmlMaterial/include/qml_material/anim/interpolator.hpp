#pragma once

#include <QtCore/QEasingCurve>
#include <QtCore/QPointF>

namespace qml_material::anim
{
inline float get_fraction_in_range(qint32 playtime, qint32 start, qint32 duration) {
    float fraction = (float)(playtime - start) / duration;
    return std::clamp(fraction, 0.f, 1.f);
}

inline auto emphasized() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    // M 0,0 C 0.05,0 0.133333,0.06 0.166666,0.4 C 0.208333,0.82 0.25,1, 1, 1)
    out.addCubicBezierSegment({ 0.05, 0 }, { 0.133333, 0.06 }, { 0.166666, 0.4 });
    out.addCubicBezierSegment({ 0.208333, 0.82 }, { 0.25, 1. }, { 1., 1. });
    return out;
}

inline auto emphasized_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.3, 0. }, { 0.8, 0.15 }, { 1., 1. });
    return out;
}

inline auto emphasized_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.05, 0.7 }, { 0.1, 1. }, { 1., 1. });
    return out;
}

inline auto standard() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.2, 0. }, { 0., 1. }, { 1., 1. });
    return out;
}

inline auto standard_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.3, 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto standard_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 0., 1. }, { 1., 1. });
    return out;
}

inline auto legacy() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}

inline auto legacy_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto legacy_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}

inline auto linear() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto fast_out_slow_in() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}

namespace linear_indicator
{
inline auto line1_head_curve() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.2, 0.0 }, { 0.8, 1.0 }, { 1.0, 1.0 });
    return out;
}
inline auto line1_tail_curve() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0.0 }, { 1.0, 1.0 }, { 1.0, 1.0 });
    return out;
}
inline auto line2_head_curve() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.0, 0.0 }, { 0.65, 1.0 }, { 1.0, 1.0 });
    return out;
}
inline auto line2_tail_curve() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.1, 0.0 }, { 0.45, 1.0 }, { 1.0, 1.0 });
    return out;
}
} // namespace linear_indicator

} // namespace qml_material::anim