#include "qml_material/util/width_provider.hpp"
#include "qml_material/core.hpp"

namespace qml_material
{
WidthProvider::WidthProvider(QObject* parent)
    : QObject(parent),
      m_column(1),
      m_width(0),
      m_total(0),
      m_minimum(160),
      m_spacing(0),
      m_left_margin(0),
      m_right_margin(0) {
    connect(
        this, &WidthProvider::totalChanged, this, &WidthProvider::refresh, Qt::DirectConnection);
}

auto WidthProvider::width() const noexcept -> double { return m_width; }
auto WidthProvider::column() const noexcept -> qint32 { return m_column; }

auto WidthProvider::minimum() const noexcept -> double { return m_minimum; }
void WidthProvider::setMinimum(double v) {
    if (! qFuzzyCompare(m_minimum, v)) {
        m_minimum = v;
        minimumChanged();
    }
}
auto WidthProvider::leftMargin() const noexcept -> double { return m_left_margin; }
void WidthProvider::setLeftMargin(double v) {
    if (! qFuzzyCompare(m_left_margin, v)) {
        m_left_margin = v;
        leftMarginChanged();
    }
}
auto WidthProvider::rightMargin() const noexcept -> double { return m_right_margin; }
void WidthProvider::setrightMargin(double v) {
    if (! qFuzzyCompare(m_right_margin, v)) {
        m_right_margin = v;
        rightMarginChanged();
    }
}

auto WidthProvider::total() const noexcept -> double { return m_total; }
void WidthProvider::setTotal(double v) {
    if (! qFuzzyCompare(m_total, v)) {
        m_total = v;
        totalChanged();
    }
}
auto WidthProvider::spacing() const noexcept -> double { return m_spacing; }
void WidthProvider::setSpacing(double v) {
    if (! qFuzzyCompare(m_spacing, v)) {
        m_spacing = v;
        spacingChanged();
    }
}

auto WidthProvider::calculateX(qint32 column) noexcept -> qint32 {
    return m_left_margin + column * (m_width + m_spacing);
}

void WidthProvider::refresh() {
    auto old   = m_width;
    auto total = m_total - m_left_margin - m_right_margin;

    if (m_minimum <= 0 || total <= 0) {
        return;
    }

    // find maxmum `count` satisfy:
    // $ count * m_minimum + (count - 1) * m_spacing <= total $
    // only keep integer
    const i64 d = (total + m_spacing) / (m_minimum + m_spacing);
    // at least one
    const i64 count = std::max<i64>(d, 1);

    m_width = static_cast<double>(total - (count - 1) * m_spacing) / count;

    if (! qFuzzyCompare(old, m_width)) {
        widthChanged();
    }

    if (count != m_column) {
        m_column = count;
        columnChanged();
    }
}

} // namespace qml_material

#include "qml_material/util/moc_width_provider.cpp"