#include "qml_material/util/image.hpp"

#include <cstdint>
#include <QImage>

namespace qml_material
{

QImage Round(QImage&& image, CornersMaskRef mask, QRect target) {
    if (target.isNull()) {
        target = QRect(QPoint(), image.size());
    } else {
        assert(QRect(QPoint(), image.size()).contains(target));
    }
    const auto targetWidth  = target.width();
    const auto targetHeight = target.height();

    image = std::move(image).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    assert(! image.isNull());

    // We need to detach image first (if it is shared), before we
    // count some offsets using QImage::bytesPerLine etc, because
    // bytesPerLine may change on detach, this leads to crashes:
    // Real image bytesPerLine is smaller than the one we use for offsets.
    auto ints = reinterpret_cast<std::uint32_t*>(image.bits());

    constexpr auto kImageIntsPerPixel = 1;
    const auto     imageIntsPerLine   = (image.bytesPerLine() >> 2);
    assert(image.depth() == ((kImageIntsPerPixel * sizeof(std::uint32_t)) << 3));
    assert(image.bytesPerLine() == (imageIntsPerLine << 2));
    const auto maskCorner = [&](const QImage* mask, bool right = false, bool bottom = false) {
        const auto maskWidth  = mask ? mask->width() : 0;
        const auto maskHeight = mask ? mask->height() : 0;
        if (! maskWidth || ! maskHeight || targetWidth < maskWidth || targetHeight < maskHeight) {
            return;
        }

        const auto maskBytesPerPixel = (mask->depth() >> 3);
        const auto maskBytesPerLine  = mask->bytesPerLine();
        const auto maskBytesAdded    = maskBytesPerLine - maskWidth * maskBytesPerPixel;
        assert(maskBytesAdded >= 0);
        assert(mask->depth() == (maskBytesPerPixel << 3));
        const auto imageIntsAdded = imageIntsPerLine - maskWidth * kImageIntsPerPixel;
        assert(imageIntsAdded >= 0);
        auto imageInts = ints + target.x() + target.y() * imageIntsPerLine;
        if (right) {
            imageInts += targetWidth - maskWidth;
        }
        if (bottom) {
            imageInts += (targetHeight - maskHeight) * imageIntsPerLine;
        }
        auto maskBytes = mask->constBits();
        for (auto y = 0; y != maskHeight; ++y) {
            for (auto x = 0; x != maskWidth; ++x) {
                // auto opacity = static_cast<anim::ShiftedMultiplier>(*maskBytes) + 1;
                // *imageInts   = anim::unshifted(anim::shifted(*imageInts) * opacity);
                // maskBytes += maskBytesPerPixel;
                // imageInts += kImageIntsPerPixel;
            }
            maskBytes += maskBytesAdded;
            imageInts += imageIntsAdded;
        }
    };

    maskCorner(mask.p[0]);
    maskCorner(mask.p[1], true);
    maskCorner(mask.p[2], false, true);
    maskCorner(mask.p[3], true, true);

    return std::move(image);
}

} // namespace qml_material