#include "qml_material/util/color_picker.hpp"

#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>

namespace qml_material
{

namespace detail
{
inline float clamp01(float x) { return std::max(0.f, std::min(1.f, x)); }
inline float wrapHue(float h) {
    while (h < 0) h += 360.f;
    while (h >= 360.f) h -= 360.f;
    return h;
}

inline float hueDistance(float a, float b) {
    float d = std::fabs(a - b);
    return std::min(d, 360.f - d);
}
} // namespace detail

static QColor extractCoolDominantColor(const QImage& src) {
    if (src.isNull()) {
        // fallback rgba(45, 70, 107, 1)
        return QColor(45, 70, 107);
    }

    QImage img = src.convertToFormat(QImage::Format_RGBA8888);

    // histogram params
    constexpr int                HUE_BINS = 36;
    constexpr float              BIN_SIZE = 360.f / HUE_BINS;
    std::array<double, HUE_BINS> hist {};
    hist.fill(0.0);

    // step sample
    const int step_x = std::max(1, img.width() / 16);
    const int step_y = std::max(1, img.height() / 16);

    double sat_sum = 0.0, sat_cnt = 0.0;

    // Constructing a hue histogram
    for (int y = 0; y < img.height(); y += step_y) {
        const uchar* row = img.constScanLine(y);
        for (int x = 0; x < img.width(); x += step_x) {
            const uchar* p = row + x * 4;

            // get hsv
            QColor     qc = QColor::fromRgb(p[0], p[1], p[2]).toHsv();
            const auto a  = p[3];

            // ignore alpha too small
            if (a < 20) continue;

            const float h = qc.hsvHueF() < 0 ? -1.f : float(qc.hsvHueF() * 360.0);
            const float s = float(qc.hsvSaturationF());
            const float v = float(qc.valueF());

            if (h < 0) {
                sat_sum += 0.0;
                sat_cnt += 1.0;
                continue;
            }

            // ignore pure white/black, like font...
            if (v < 0.05f || v > 0.98f) continue;

            // use s*v
            float w = s * v;
            w       = std::pow(w, 1.2f);

            // more like cool color
            if (h >= 180.f && h <= 300.f) {
                w *= 1.35f;
            }

            // lower warm color
            if ((h >= 5.f && h <= 50.f)) {
                w *= 0.75f;
            }

            int bin = int(h / BIN_SIZE);
            if (bin >= HUE_BINS) bin = HUE_BINS - 1;
            hist[bin] += w;

            sat_sum += s;
            sat_cnt += 1.0;
        }
    }

    // near gray, no color
    const double avg_sat = (sat_cnt > 0.0) ? (sat_sum / sat_cnt) : 0.0;
    if (sat_cnt < 200 || avg_sat < 0.06) {
        // to blue
        return QColor::fromHslF(210.0 / 360.0, 0.12, 0.28);
    }

    // find peak bin
    int   peakBin = int(std::max_element(hist.begin(), hist.end()) - hist.begin());
    float peakHue = (peakBin + 0.5f) * BIN_SIZE;

    // find weighted average color near peak (±25°)
    double W  = 0.0;
    double Rs = 0.0, Gs = 0.0, Bs = 0.0;
    for (int y = 0; y < img.height(); y += step_y) {
        const uchar* row = img.constScanLine(y);
        for (int x = 0; x < img.width(); x += step_x) {
            const uchar* p = row + x * 4;
            const int    r = p[0], g = p[1], b = p[2], a = p[3];
            if (a < 20) continue;

            QColor      qc = QColor::fromRgb(r, g, b);
            const float h  = qc.hsvHueF() < 0 ? -1.f : float(qc.hsvHueF() * 360.0);
            const float s  = float(qc.hsvSaturationF());
            const float v  = float(qc.valueF());
            if (h < 0) continue;
            if (v < 0.05f || v > 0.98f) continue;

            const float dh = detail::hueDistance(h, peakHue);
            if (dh > 25.f) continue;

            // weight: s*v * exp(-(dh²)/(2*σ²)), σ≈12°
            float w = s * v * std::exp(-(dh * dh) / (2.f * 12.f * 12.f)); // σ≈12°
            w       = std::pow(w, 1.15f);
            W += w;
            Rs += w * r;
            Gs += w * g;
            Bs += w * b;
        }
    }

    QColor result;
    if (W > 0.0) {
        int R  = int(std::round(Rs / W));
        int G  = int(std::round(Gs / W));
        int B  = int(std::round(Bs / W));
        result = QColor(R, G, B);
    } else {
        // backup: s=0.25, l=0.32
        result = QColor::fromHslF(peakHue / 360.f, 0.25, 0.32);
    }

    // to HSL space for adjustment
    float h, s, l, a;
    result.getHslF(&h, &s, &l, &a);

    // make hue closer to 210°
    const float targetHue = 210.f / 360.f;
    h                     = detail::wrapHue(float(h * 360.f));
    float h_deg           = float(h);

    // select the shortest circular direction to advance
    float diff = targetHue * 360.f - h_deg;
    if (diff > 180.f) diff -= 360.f;
    if (diff < -180.f) diff += 360.f;
    h_deg = detail::wrapHue(h_deg + 0.2f * diff);
    h     = h_deg / 360.f;

    // little more saturation, little less lightness
    s = std::clamp(s + 0.06f, 0.0f, 1.0f);
    l = std::clamp(l * 0.92f, 0.0f, 1.0f);

    // keep not too dark nor too light
    l = std::clamp(l, 0.20f, 0.40f);

    result.setHslF(h, s, l, a);
    return result;
}

void ColorPickerRunnable::run() {
    auto color = extractCoolDominantColor(image);
    finished(color);
}

ColorPicker::ColorPicker(QObject* parent): QObject(parent) {
    connect(this, &ColorPicker::pick, [this] {
        auto pick = new ColorPickerRunnable();
        pick->setAutoDelete(true);
        pick->image = m_image;
        connect(pick, &ColorPickerRunnable::finished, this, &ColorPicker::setColor);
        QThreadPool::globalInstance()->start(pick);
    });
}
ColorPicker::~ColorPicker() {}

auto ColorPicker::color() const noexcept -> QColor { return m_color; }
void ColorPicker::setColor(QColor c) {
    if (m_color != c) {
        m_color = c;
        colorChanged();
    }
}
auto ColorPicker::image() const -> QImage { return m_image; }
void ColorPicker::setImage(QImage img) {
    if (m_image != img) {
        m_image = img;
        imageChanged();
    }
}

} // namespace qml_material

#include "qml_material/util/moc_color_picker.cpp"