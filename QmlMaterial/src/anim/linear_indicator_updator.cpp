#include "qml_material/anim/linear_indicator_updator.hpp"

#include "qml_material/anim/interpolator.hpp"

// animation data from:
// https://github.com/material-components/material-components-android/blob/release-1.13/lib/java/com/google/android/material/progressindicator/LinearIndeterminateDisjointAnimatorDelegate.java
// https://github.com/material-components/material-components-android/blob/release-1.13/lib/java/com/google/android/material/progressindicator/LinearIndeterminateContiguousAnimatorDelegate.java

namespace
{
namespace disjoint
{
constexpr i32        TOTAL_DURATION_IN_MS          = 1800;
constexpr std::array DURATION_TO_MOVE_SEGMENT_ENDS = { 533, 567, 850, 750 };
constexpr std::array DELAY_TO_MOVE_SEGMENT_ENDS    = { 1267, 1000, 333, 0 };
constexpr i32        COUNT                         = 2;

} // namespace disjoint

namespace contiguous
{
constexpr i32 TOTAL_DURATION_IN_MS     = 667;
constexpr i32 DURATION_PER_CYCLE_IN_MS = 333;
constexpr i32 COUNT                    = 3;
} // namespace contiguous
} // namespace

namespace qml_material
{

LinearActiveIndicatorData::LinearActiveIndicatorData(QObject* parent): QObject(parent) {}
auto LinearActiveIndicatorData::startFraction() const noexcept -> double { return start_fraction; }
auto LinearActiveIndicatorData::endFraction() const noexcept -> double { return end_fraction; }
auto LinearActiveIndicatorData::getColor() const noexcept -> QColor { return color; }
auto LinearActiveIndicatorData::getGapSize() const noexcept -> qint32 { return gap_size; }

LinearIndicatorUpdator::LinearIndicatorUpdator(QObject* parent)
    : QObject(parent),
      m_type(IndeterminateAnimationType::DisJoint),
      m_contiguous_interpolator(anim::fast_out_slow_in()),
      m_interpolators({ anim::linear_indicator::line1_head_curve(),
                        anim::linear_indicator::line1_tail_curve(),
                        anim::linear_indicator::line2_head_curve(),
                        anim::linear_indicator::line2_tail_curve() }),
      m_progress(0),
      m_complete_end_progress(0),
      m_color_idx(0),
      m_color_dirty(true) {
    m_active_indicators = {
        new LinearActiveIndicatorData(this),
        new LinearActiveIndicatorData(this),
        new LinearActiveIndicatorData(this),
    };
    for (auto el : m_active_indicators) {
        connect(this, &LinearIndicatorUpdator::updated, el, &LinearActiveIndicatorData::updated);
    }
    connect(this,
            &LinearIndicatorUpdator::indeterminateAnimationTypeChanged,
            this,
            &LinearIndicatorUpdator::initIndicatorDatas);
    connect(this,
            &LinearIndicatorUpdator::colorsChanged,
            this,
            &LinearIndicatorUpdator::initIndicatorDatas);
    initIndicatorDatas();
}

auto LinearIndicatorUpdator::activeIndicators() -> QList<LinearActiveIndicatorData*> {
    auto start = m_active_indicators.begin();
    if (m_type == IndeterminateAnimationType::DisJoint) {
        return { start, start + disjoint::COUNT };
    } else {
        return { start, start + contiguous::COUNT };
    }
}

auto LinearIndicatorUpdator::progress() const noexcept -> double { return m_progress; }
auto LinearIndicatorUpdator::duration() const noexcept -> double {
    if (m_type == IndeterminateAnimationType::DisJoint) {
        return disjoint::TOTAL_DURATION_IN_MS;
    } else {
        return contiguous::DURATION_PER_CYCLE_IN_MS;
    }
}
auto LinearIndicatorUpdator::completeEndDuration() const noexcept -> double {
    if (m_type == IndeterminateAnimationType::DisJoint) {
        return disjoint::TOTAL_DURATION_IN_MS;
    } else {
        return contiguous::TOTAL_DURATION_IN_MS;
    }
}
auto LinearIndicatorUpdator::colors() const -> QList<QColor> { return m_colors; }
void LinearIndicatorUpdator::setColors(const QList<QColor>& v) {
    if (m_colors != v) {
        m_colors    = v;
        m_color_idx = m_color_idx % m_colors.size();
        colorsChanged();
    }
}

auto LinearIndicatorUpdator::indeterminateAnimationType() const noexcept
    -> IndeterminateAnimationType {
    return m_type;
}
void LinearIndicatorUpdator::setIndeterminateAnimationType(IndeterminateAnimationType t) {
    if (m_type != t) {
        m_type = t;
        indeterminateAnimationTypeChanged();
    }
}

auto LinearIndicatorUpdator::completeEndProgress() const noexcept -> double {
    return m_complete_end_progress;
}

void LinearIndicatorUpdator::updateCompleteEndProgress(double progress) {
    m_complete_end_progress = progress;
    update(m_progress);
}

void LinearIndicatorUpdator::updateContiguous(double progress) noexcept {
    using namespace contiguous;
    auto playtime = progress * DURATION_PER_CYCLE_IN_MS;

    m_active_indicators[0]->start_fraction = 0.f;
    float fraction = anim::get_fraction_in_range(playtime, 0, TOTAL_DURATION_IN_MS);

    m_active_indicators[0]->end_fraction = m_active_indicators[1]->start_fraction =
        m_contiguous_interpolator.valueForProgress(fraction);

    fraction += (float)DURATION_PER_CYCLE_IN_MS / TOTAL_DURATION_IN_MS;
    m_active_indicators[1]->end_fraction = m_active_indicators[2]->start_fraction =
        m_contiguous_interpolator.valueForProgress(fraction);

    m_active_indicators[2]->end_fraction = 1.f;
}
void LinearIndicatorUpdator::updateDisjoint(double progress) noexcept {
    using namespace disjoint;
    auto playtime = progress * TOTAL_DURATION_IN_MS;
    for (usize i = 0; i < 2; i++) {
        auto& a        = *m_active_indicators[i];
        auto  fraction = anim::get_fraction_in_range(
            playtime, DELAY_TO_MOVE_SEGMENT_ENDS[2 * i], DURATION_TO_MOVE_SEGMENT_ENDS[2 * i]);

        a.start_fraction = std::clamp(m_interpolators[2 * i].valueForProgress(fraction), 0.0, 1.0);

        fraction = anim::get_fraction_in_range(playtime,
                                               DELAY_TO_MOVE_SEGMENT_ENDS[2 * i + 1],
                                               DURATION_TO_MOVE_SEGMENT_ENDS[2 * i + 1]);
        a.end_fraction =
            std::clamp(m_interpolators[2 * i + 1].valueForProgress(fraction), 0.0, 1.0);
    }
}
void LinearIndicatorUpdator::updateColors() noexcept {
    if (m_colors.empty()) return;
    if (! m_color_dirty) return;

    auto color = m_colors[m_color_idx];
    if (m_type == IndeterminateAnimationType::DisJoint) {
        for (auto el : m_active_indicators) {
            el->color = color;
        }
        m_color_dirty = false;
    } else {
        auto& a = m_active_indicators;
        if (a[1]->end_fraction < 1.f) {
            a[2]->color   = a[1]->color;
            a[1]->color   = a[0]->color;
            a[0]->color   = color;
            m_color_dirty = false;
        }
    }
}

void LinearIndicatorUpdator::initIndicatorDatas() noexcept {
    for (auto& el : m_active_indicators) {
        el->gap_size = 4;
        if (! m_colors.empty()) {
            el->color = m_colors[0];
        }
    }
}

void LinearIndicatorUpdator::update(double progress) {
    if (m_type == IndeterminateAnimationType::DisJoint) {
        updateDisjoint(progress);
    } else {
        updateContiguous(progress);
    }
    if (m_progress > progress) {
        m_color_idx   = (m_color_idx + 1) % m_colors.size();
        m_color_dirty = true;
    }
    if (m_color_dirty) updateColors();
    m_progress = progress;
    updated();
}

} // namespace qml_material

#include "qml_material/anim/moc_linear_indicator_updator.cpp"