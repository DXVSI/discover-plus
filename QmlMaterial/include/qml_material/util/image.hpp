#include <span>
#include <QImage>

namespace qml_material
{

struct CornersMaskRef {
    CornersMaskRef() = default;
    explicit CornersMaskRef(std::span<const QImage, 4> masks)
        : p { &masks[0], &masks[1], &masks[2], &masks[3] } {}
    explicit CornersMaskRef(std::array<const QImage, 4> masks)
        : p { &masks[0], &masks[1], &masks[2], &masks[3] } {}
    explicit CornersMaskRef(std::span<const QImage*, 4> masks)
        : p { masks[0], masks[1], masks[2], masks[3] } {}
    explicit CornersMaskRef(std::array<const QImage*, 4> masks)
        : p { masks[0], masks[1], masks[2], masks[3] } {}

    [[nodiscard]] bool empty() const { return ! p[0] && ! p[1] && ! p[2] && ! p[3]; }

    std::array<const QImage*, 4> p {};

    friend inline constexpr std::strong_ordering operator<=>(CornersMaskRef a,
                                                             CornersMaskRef b) noexcept {
        for (auto i = 0; i != 4; ++i) {
            if (a.p[i] < b.p[i]) {
                return std::strong_ordering::less;
            } else if (a.p[i] > b.p[i]) {
                return std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }
    friend inline constexpr bool operator==(CornersMaskRef a, CornersMaskRef b) noexcept = default;
};

} // namespace qml_image