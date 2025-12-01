#include "qml_material/anim/circular_indicator_updator.hpp"
#include "qml_material/anim/interpolator.hpp"

// animation data from:
// https://github.com/material-components/material-components-android/blob/release-1.13/lib/java/com/google/android/material/progressindicator/CircularProgressIndicator.java
// https://github.com/material-components/material-components-android/blob/release-1.13/lib/java/com/google/android/material/progressindicator/CircularIndeterminateAdvanceAnimatorDelegate.java

namespace
{

namespace advance
{
constexpr i32 TOTAL_CYCLES                   = 4;
constexpr i32 TOTAL_DURATION_IN_MS           = 5400;
constexpr i32 DURATION_TO_EXPAND_IN_MS       = 667;
constexpr i32 DURATION_TO_COLLAPSE_IN_MS     = 667;
constexpr i32 DURATION_TO_FADE_IN_MS         = 333;
constexpr i32 DURATION_TO_COMPLETE_END_IN_MS = 333;
constexpr i32 TAIL_DEGREES_OFFSET            = -20;
constexpr i32 EXTRA_DEGREES_PER_CYCLE        = 250;
constexpr i32 CONSTANT_ROTATION_DEGREES      = 1520;

constexpr std::array<i32, TOTAL_CYCLES> DELAY_TO_EXPAND_IN_MS   = { 0, 1350, 2700, 4050 };
constexpr std::array<i32, TOTAL_CYCLES> DELAY_TO_COLLAPSE_IN_MS = { 667, 2017, 3367, 4717 };
constexpr std::array<i32, TOTAL_CYCLES> DELAY_TO_FADE_IN_MS     = { 1000, 2350, 3700, 5050 };
} // namespace advance

namespace retreat
{

constexpr i32        TOTAL_DURATION_IN_MS           = 6000;
constexpr i32        DURATION_SPIN_IN_MS            = 500;
constexpr i32        DURATION_GROW_ACTIVE_IN_MS     = 3000;
constexpr i32        DURATION_SHRINK_ACTIVE_IN_MS   = 3000;
constexpr std::array DELAY_SPINS_IN_MS              = { 0, 1500, 3000, 4500 };
constexpr i32        DELAY_GROW_ACTIVE_IN_MS        = 0;
constexpr i32        DELAY_SHRINK_ACTIVE_IN_MS      = 3000;
constexpr i32        DURATION_TO_COMPLETE_END_IN_MS = 500;
constexpr i32        DURATION_TO_FADE_IN_MS         = 100;

// Constants for animation values.

// The total degrees that a constant rotation goes by.
constexpr i32 CONSTANT_ROTATION_DEGREES = 1080;
// Despite of the constant rotation, there are also 5 extra rotations the entire animation. The
// total degrees that each extra rotation goes by.
constexpr i32        SPIN_ROTATION_DEGREES = 90;
constexpr float      START_FRACTION        = 0.f;
constexpr std::array END_FRACTION_RANGE    = { 0.10f, 0.87f };

} // namespace retreat

} // namespace

namespace qml_material
{

CircularIndicatorUpdator::CircularIndicatorUpdator(QObject* parent)
    : QObject(parent),
      m_type(IndeterminateAnimationType::Advance),
      m_curve(anim::fast_out_slow_in()),
      m_progress(0),
      m_start_fraction(0),
      m_end_fraction(0),
      m_rotation_degree(0),
      m_complete_end_progress(0) {}

auto CircularIndicatorUpdator::startFraction() const noexcept -> double { return m_start_fraction; }
auto CircularIndicatorUpdator::endFraction() const noexcept -> double { return m_end_fraction; }
auto CircularIndicatorUpdator::rotation() const noexcept -> double { return m_rotation_degree; }
auto CircularIndicatorUpdator::progress() const noexcept -> double { return m_progress; }
auto CircularIndicatorUpdator::duration() const noexcept -> double {
    if (m_type == IndeterminateAnimationType::Advance) {
        return advance::TOTAL_DURATION_IN_MS;
    } else {
        return retreat::TOTAL_DURATION_IN_MS;
    }
}
auto CircularIndicatorUpdator::completeEndDuration() const noexcept -> double {
    if (m_type == IndeterminateAnimationType::Advance) {
        return advance::DURATION_TO_COMPLETE_END_IN_MS;
    } else {
        return retreat::DURATION_TO_COMPLETE_END_IN_MS;
    }
}
auto CircularIndicatorUpdator::indeterminateAnimationType() const noexcept
    -> IndeterminateAnimationType {
    return m_type;
}
void CircularIndicatorUpdator::setIndeterminateAnimationType(IndeterminateAnimationType t) {
    if (m_type != t) {
        m_type = t;
        indeterminateAnimationTypeChanged();
    }
}

auto CircularIndicatorUpdator::completeEndProgress() const noexcept -> double {
    return m_complete_end_progress;
}
void CircularIndicatorUpdator::updateCompleteEndProgress(double progress) {
    m_complete_end_progress = progress;
    update(m_progress);
}
void CircularIndicatorUpdator::update(double progress) {
    if (m_type == IndeterminateAnimationType::Advance) {
        updateAdvance(progress);
    } else {
        updateRetreat(progress);
    }
    m_progress = progress;
    updated();
}
void CircularIndicatorUpdator::updateRetreat(double progress) noexcept {
    using namespace retreat;
    auto playtime = progress * TOTAL_DURATION_IN_MS;
    // Constant rotation.
    float constantRotation = CONSTANT_ROTATION_DEGREES * progress;
    // Extra rotation for the faster spinning.
    float spinRotation = 0;
    for (int spinDelay : DELAY_SPINS_IN_MS) {
        spinRotation +=
            m_curve.valueForProgress(anim::get_fraction_in_range(playtime, spinDelay, DURATION_SPIN_IN_MS)) *
            SPIN_ROTATION_DEGREES;
    }
    m_rotation_degree = constantRotation + spinRotation;
    // Grow active indicator.
    float fraction = m_curve.valueForProgress(
        anim::get_fraction_in_range(playtime, DELAY_GROW_ACTIVE_IN_MS, DURATION_GROW_ACTIVE_IN_MS));
    fraction -= m_curve.valueForProgress(
        anim::get_fraction_in_range(playtime, DELAY_SHRINK_ACTIVE_IN_MS, DURATION_SHRINK_ACTIVE_IN_MS));
    m_start_fraction = START_FRACTION;
    m_end_fraction   = std::lerp(END_FRACTION_RANGE[0], END_FRACTION_RANGE[1], fraction);

    // Completing animation.
    if (m_complete_end_progress > 0) {
        m_end_fraction *= 1 - m_complete_end_progress;
    }
}

void CircularIndicatorUpdator::updateAdvance(double progress) noexcept {
    using namespace advance;
    const i64 playtime = progress * TOTAL_DURATION_IN_MS;

    // Adds constant rotation to segment positions.
    m_start_fraction = CONSTANT_ROTATION_DEGREES * progress + TAIL_DEGREES_OFFSET;
    m_end_fraction   = CONSTANT_ROTATION_DEGREES * progress;

    // Adds cycle specific rotation to segment positions.
    for (i32 cycleIndex = 0; cycleIndex < TOTAL_CYCLES; cycleIndex++) {
        // While expanding.
        float fraction = anim::get_fraction_in_range(
            playtime, DELAY_TO_EXPAND_IN_MS[cycleIndex], DURATION_TO_EXPAND_IN_MS);
        m_end_fraction += m_curve.valueForProgress(fraction) * EXTRA_DEGREES_PER_CYCLE;

        // While collapsing.
        fraction = anim::get_fraction_in_range(
            playtime, DELAY_TO_COLLAPSE_IN_MS[cycleIndex], DURATION_TO_COLLAPSE_IN_MS);
        m_start_fraction += m_curve.valueForProgress(fraction) * EXTRA_DEGREES_PER_CYCLE;
    }

    // Closes the gap between head and tail for complete end.
    m_start_fraction += (m_end_fraction - m_start_fraction) * m_complete_end_progress;

    m_start_fraction /= 360.f;
    m_end_fraction /= 360.f;
}

} // namespace qml_material

#include "qml_material/anim/moc_circular_indicator_updator.cpp"