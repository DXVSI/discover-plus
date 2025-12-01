/*
 *  SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kirigami/wheelhandler.h"
// #include "settings.h"
// #include <libkirigami/units.h>

#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWheelEvent>

namespace
{
bool fuzzyLessThanOrEqualTo(qreal a, qreal b) {
    if (a == 0.0 || b == 0.0) {
        // qFuzzyCompare is broken
        a += 1.0;
        b += 1.0;
    }
    return a <= b || qFuzzyCompare(a, b);
}

} // namespace

KirigamiWheelEvent::KirigamiWheelEvent(QObject* parent): QObject(parent) {}

KirigamiWheelEvent::~KirigamiWheelEvent() {}

void KirigamiWheelEvent::initializeFromEvent(QWheelEvent* event) {
    m_x          = event->position().x();
    m_y          = event->position().y();
    m_angleDelta = event->angleDelta();
    m_pixelDelta = event->pixelDelta();
    m_buttons    = event->buttons();
    m_modifiers  = event->modifiers();
    m_accepted   = false;
    m_inverted   = event->inverted();
}

qreal KirigamiWheelEvent::x() const { return m_x; }

qreal KirigamiWheelEvent::y() const { return m_y; }

QPointF KirigamiWheelEvent::angleDelta() const { return m_angleDelta; }

QPointF KirigamiWheelEvent::pixelDelta() const { return m_pixelDelta; }

int KirigamiWheelEvent::buttons() const { return m_buttons; }

int KirigamiWheelEvent::modifiers() const { return m_modifiers; }

bool KirigamiWheelEvent::inverted() const { return m_inverted; }

bool KirigamiWheelEvent::isAccepted() { return m_accepted; }

void KirigamiWheelEvent::setAccepted(bool accepted) { m_accepted = accepted; }

///////////////////////////////

WheelFilterItem::WheelFilterItem(QQuickItem* parent): QQuickItem(parent) { setEnabled(false); }

///////////////////////////////

WheelHandler::WheelHandler(QObject* parent): QObject(parent), m_active(true) {
    m_wheelScrollingTimer.setSingleShot(true);
    m_wheelScrollingTimer.setInterval(m_wheelScrollingDuration);
    m_wheelScrollingTimer.callOnTimeout([this]() {
        setScrolling(false);
    });

    connect(QGuiApplication::styleHints(),
            &QStyleHints::wheelScrollLinesChanged,
            this,
            [this](int scrollLines) {
                m_defaultPixelStepSize = 20 * scrollLines;
                if (! m_explicitVStepSize && m_verticalStepSize != m_defaultPixelStepSize) {
                    m_verticalStepSize = m_defaultPixelStepSize;
                    Q_EMIT verticalStepSizeChanged();
                }
                if (! m_explicitHStepSize && m_horizontalStepSize != m_defaultPixelStepSize) {
                    m_horizontalStepSize = m_defaultPixelStepSize;
                    Q_EMIT horizontalStepSizeChanged();
                }
            });
}

WheelHandler::~WheelHandler() {}

QQuickItem* WheelHandler::target() const { return m_target; }

void WheelHandler::setTarget(QQuickItem* target) {
#ifdef __ANDROID__
    return;
#endif
    if (m_target == target) {
        return;
    }

    if (target && ! target->inherits("QQuickFlickable")) {
        qmlWarning(this) << "target must be a QQuickFlickable";
        return;
    }

    if (m_target) {
        // disconnect(m_flickable, nullptr, m_filterItem, nullptr);
        // disconnect( m_flickable, &QQuickItem::parentChanged, this,
        // &WheelHandler::_k_rebindScrollBars);
        detach();
    }

    m_target = target;

    const auto qCtx = qmlContext(m_target);
    assert(qCtx);

    m_flickable.originX = QQmlProperty(m_target, QStringLiteral("originX"), qCtx);
    m_flickable.originY = QQmlProperty(m_target, QStringLiteral("originY"), qCtx);

    m_flickable.leftMargin   = QQmlProperty(m_target, QStringLiteral("leftMargin"), qCtx);
    m_flickable.rightMargin  = QQmlProperty(m_target, QStringLiteral("rightMargin"), qCtx);
    m_flickable.topMargin    = QQmlProperty(m_target, QStringLiteral("topMargin"), qCtx);
    m_flickable.bottomMargin = QQmlProperty(m_target, QStringLiteral("bottomMargin"), qCtx);

    m_flickable.contentX      = QQmlProperty(m_target, QStringLiteral("contentX"), qCtx);
    m_flickable.contentY      = QQmlProperty(m_target, QStringLiteral("contentY"), qCtx);
    m_flickable.contentHeight = QQmlProperty(m_target, QStringLiteral("contentHeight"), qCtx);
    m_flickable.contentWidth  = QQmlProperty(m_target, QStringLiteral("contentWidth"), qCtx);
    m_flickable.height        = QQmlProperty(m_target, QStringLiteral("height"), qCtx);
    m_flickable.width         = QQmlProperty(m_target, QStringLiteral("width"), qCtx);
    m_flickable.interactive   = QQmlProperty(m_target, QStringLiteral("interactive"), qCtx);

    m_scrollBarV.scrollBar = QQmlProperty(m_target, QStringLiteral("ScrollBar.vertical"), qCtx);
    m_scrollBarH.scrollBar = QQmlProperty(m_target, QStringLiteral("ScrollBar.horizontal"), qCtx);

    m_flickable.interactive.connectNotifySignal(this, SLOT(refreshAttach()));
    m_scrollBarV.scrollBar.connectNotifySignal(this, SLOT(rebindScrollBarV()));
    m_scrollBarH.scrollBar.connectNotifySignal(this, SLOT(rebindScrollBarH()));

    refreshAttach();
    /*
    m_filterItem->setParentItem(target);
    if (target) {
        target->installEventFilter(this);

        // Stack WheelFilterItem over the Flickable's scrollable content
        m_filterItem->stackAfter(target->property("contentItem").value<QQuickItem*>());
        // Make it fill the Flickable
        m_filterItem->setWidth(target->width());
        m_filterItem->setHeight(target->height());
        connect(target, &QQuickItem::widthChanged, m_filterItem, [this, target]() {
            m_filterItem->setWidth(target->width());
        });
        connect(target, &QQuickItem::heightChanged, m_filterItem, [this, target]() {
            m_filterItem->setHeight(target->height());
        });
    }
    */
    rebindScrollBarH();
    rebindScrollBarV();
    Q_EMIT targetChanged();
}

void WheelHandler::refreshAttach() {
    if (m_flickable.interactive.read().toBool()) {
        attach();
    } else {
        detach();
    }
}

void WheelHandler::attach() {
    if (m_target) {
        m_target->installEventFilter(this);
    }
}
void WheelHandler::detach() {
    if (m_target) {
        m_target->removeEventFilter(this);
    }
}

void WheelHandler::rebindScrollBar(ScrollBar& scrollBar) {
    const auto item = scrollBar.scrollBar.read().value<QQuickItem*>();

    scrollBar.item = item;

    if (item) {
        scrollBar.stepSize = QQmlProperty(item, QStringLiteral("stepSize"), qmlContext(item));
        scrollBar.decreaseMethod =
            item->metaObject()->method(item->metaObject()->indexOfMethod("decrease()"));
        scrollBar.increaseMethod =
            item->metaObject()->method(item->metaObject()->indexOfMethod("increase()"));
    }
}
auto WheelHandler::active() const -> bool { return m_active; }
void WheelHandler::setActive(bool v) {
    if (m_active != v) {
        m_active = v;
        activeChanged();
    }
}
qreal WheelHandler::verticalStepSize() const { return m_verticalStepSize; }

void WheelHandler::setVerticalStepSize(qreal stepSize) {
    m_explicitVStepSize = true;
    if (qFuzzyCompare(m_verticalStepSize, stepSize)) {
        return;
    }
    // Mimic the behavior of QQuickScrollBar when stepSize is 0
    if (qFuzzyIsNull(stepSize)) {
        resetVerticalStepSize();
        return;
    }
    m_verticalStepSize = stepSize;
    Q_EMIT verticalStepSizeChanged();
}

void WheelHandler::resetVerticalStepSize() {
    m_explicitVStepSize = false;
    if (qFuzzyCompare(m_verticalStepSize, m_defaultPixelStepSize)) {
        return;
    }
    m_verticalStepSize = m_defaultPixelStepSize;
    Q_EMIT verticalStepSizeChanged();
}

qreal WheelHandler::horizontalStepSize() const { return m_horizontalStepSize; }

void WheelHandler::setHorizontalStepSize(qreal stepSize) {
    m_explicitHStepSize = true;
    if (qFuzzyCompare(m_horizontalStepSize, stepSize)) {
        return;
    }
    // Mimic the behavior of QQuickScrollBar when stepSize is 0
    if (qFuzzyIsNull(stepSize)) {
        resetHorizontalStepSize();
        return;
    }
    m_horizontalStepSize = stepSize;
    Q_EMIT horizontalStepSizeChanged();
}

void WheelHandler::resetHorizontalStepSize() {
    m_explicitHStepSize = false;
    if (qFuzzyCompare(m_horizontalStepSize, m_defaultPixelStepSize)) {
        return;
    }
    m_horizontalStepSize = m_defaultPixelStepSize;
    Q_EMIT horizontalStepSizeChanged();
}

Qt::KeyboardModifiers WheelHandler::pageScrollModifiers() const { return m_pageScrollModifiers; }

void WheelHandler::setPageScrollModifiers(Qt::KeyboardModifiers modifiers) {
    if (m_pageScrollModifiers == modifiers) {
        return;
    }
    m_pageScrollModifiers = modifiers;
    Q_EMIT pageScrollModifiersChanged();
}

void WheelHandler::resetPageScrollModifiers() {
    setPageScrollModifiers(m_defaultPageScrollModifiers);
}

bool WheelHandler::filterMouseEvents() const { return m_filterMouseEvents; }

void WheelHandler::setFilterMouseEvents(bool enabled) {
    if (m_filterMouseEvents == enabled) {
        return;
    }
    m_filterMouseEvents = enabled;
    Q_EMIT filterMouseEventsChanged();
}

bool WheelHandler::keyNavigationEnabled() const { return m_keyNavigationEnabled; }

void WheelHandler::setKeyNavigationEnabled(bool enabled) {
    if (m_keyNavigationEnabled == enabled) {
        return;
    }
    m_keyNavigationEnabled = enabled;
    Q_EMIT keyNavigationEnabledChanged();
}

void WheelHandler::classBegin() {
    // Initializes smooth scrolling
    m_engine = qmlEngine(this);
}

void WheelHandler::componentComplete() {}

void WheelHandler::setScrolling(bool scrolling) {
    if (m_wheelScrolling == scrolling) {
        if (m_wheelScrolling) {
            m_wheelScrollingTimer.start();
        }
        return;
    }
    m_wheelScrolling = scrolling;
}

bool WheelHandler::scrollFlickable(QPointF pixelDelta, QPointF angleDelta,
                                   Qt::KeyboardModifiers modifiers) {
    if (! m_target || (pixelDelta.isNull() && angleDelta.isNull())) {
        return false;
    }

    bool scrolled { false };

    auto handler = [this, modifiers, &pixelDelta, &angleDelta, &scrolled](Qt::Orientation ori) {
        bool        hr { ori == Qt::Horizontal };
        const qreal size =
            hr ? m_flickable.width.read().toReal() : m_flickable.height.read().toReal();
        const qreal contentSize = hr ? m_flickable.contentWidth.read().toReal()
                                     : m_flickable.contentHeight.read().toReal();
        const qreal contentPos =
            hr ? m_flickable.contentX.read().toReal() : m_flickable.contentY.read().toReal();
        const qreal begginMargin =
            hr ? m_flickable.leftMargin.read().toReal() : m_flickable.topMargin.read().toReal();
        const qreal endMargin =
            hr ? m_flickable.rightMargin.read().toReal() : m_flickable.bottomMargin.read().toReal();
        const qreal originPos =
            hr ? m_flickable.originX.read().toReal() : m_flickable.originY.read().toReal();
        const qreal pageSize = size - begginMargin - endMargin;
        const auto  window   = m_target->window();
        const qreal devicePixelRatio =
            window != nullptr ? window->devicePixelRatio() : qGuiApp->devicePixelRatio();

        const qreal minExtent = originPos - begginMargin;
        const qreal maxExtent =
            qMax<qreal>(minExtent, (contentSize + begginMargin + originPos) - size);

        const bool atBeginning = fuzzyLessThanOrEqualTo(contentPos, minExtent);
        const bool atEnd       = fuzzyLessThanOrEqualTo(maxExtent, contentPos);

        // HACK: Only transpose deltas when not using xcb in order to not conflict with xcb's own
        // delta transposing
        if (modifiers & m_defaultHorizontalScrollModifiers &&
            qGuiApp->platformName() != QLatin1String("xcb")) {
            angleDelta = angleDelta.transposed();
            pixelDelta = pixelDelta.transposed();
        }

        const qreal ticks = (hr ? angleDelta.x() : angleDelta.y()) / 120;
        qreal       change;
        auto        stepSize  = hr ? m_horizontalStepSize : m_verticalStepSize;
        auto&       scrollBar = hr ? m_scrollBarH : m_scrollBarV;

        auto& contentPosProp = hr ? m_flickable.contentX : m_flickable.contentY;

        if (contentSize > pageSize) {
            // Use page size with pageScrollModifiers. Matches QScrollBar, which uses
            // QAbstractSlider behavior.
            if (modifiers & m_pageScrollModifiers) {
                change = qBound(-pageSize, ticks * pageSize, pageSize);
            } else if (pixelDelta.x() != 0) {
                change = pixelDelta.x();
            } else {
                change = ticks * stepSize;
            }

            if (scrollBar.valid()) {
                if (((change > 0 && ! atBeginning) || (change < 0 && ! atEnd))) {
                    const auto _stepSize = scrollBar.stepSize.read().toReal();
                    scrollBar.stepSize.write(qBound<qreal>(0, std::abs(change) / contentSize, 1));
                    if (change > 0)
                        scrollBar.decreaseMethod.invoke(scrollBar.item);
                    else
                        scrollBar.increaseMethod.invoke(scrollBar.item);
                    scrollBar.stepSize.write(_stepSize);
                    scrolled = true;
                }
            } else {
                // contentX and contentY use reversed signs from what x and y would normally use, so
                // flip the signs

                qreal newContentPos = std::clamp(contentPos - change, minExtent, maxExtent);

                // Flickable::pixelAligned rounds the position, so round to mimic that behavior.
                // Rounding prevents fractional positioning from causing text to be
                // clipped off on the top and bottom.
                // Multiply by devicePixelRatio before rounding and divide by devicePixelRatio
                // after to make position match pixels on the screen more closely.
                newContentPos = std::round(newContentPos * devicePixelRatio) / devicePixelRatio;
                if (contentPos != newContentPos) {
                    scrolled = true;
                    contentPosProp.write(newContentPos);
                }
            }
        }
    };

    handler(Qt::Horizontal);
    handler(Qt::Vertical);

    return scrolled;
}

bool WheelHandler::scrollUp(qreal stepSize) {
    if (qFuzzyIsNull(stepSize)) {
        return false;
    } else if (stepSize < 0) {
        stepSize = m_verticalStepSize;
    }
    // contentY uses reversed sign
    return scrollFlickable(QPointF(0, stepSize));
}

bool WheelHandler::scrollDown(qreal stepSize) {
    if (qFuzzyIsNull(stepSize)) {
        return false;
    } else if (stepSize < 0) {
        stepSize = m_verticalStepSize;
    }
    // contentY uses reversed sign
    return scrollFlickable(QPointF(0, -stepSize));
}

bool WheelHandler::scrollLeft(qreal stepSize) {
    if (qFuzzyIsNull(stepSize)) {
        return false;
    } else if (stepSize < 0) {
        stepSize = m_horizontalStepSize;
    }
    // contentX uses reversed sign
    return scrollFlickable(QPoint(stepSize, 0));
}

bool WheelHandler::scrollRight(qreal stepSize) {
    if (qFuzzyIsNull(stepSize)) {
        return false;
    } else if (stepSize < 0) {
        stepSize = m_horizontalStepSize;
    }
    // contentX uses reversed sign
    return scrollFlickable(QPoint(-stepSize, 0));
}

bool WheelHandler::eventFilter(QObject* watched, QEvent* event) {
    auto item = qobject_cast<QQuickItem*>(watched);
    if (! item || ! item->isEnabled() || ! m_active) {
        return false;
    }

    qreal contentWidth  = 0;
    qreal contentHeight = 0;
    qreal pageWidth     = 0;
    qreal pageHeight    = 0;
    if (m_target) {
        contentWidth  = m_flickable.contentWidth.read().toReal();
        contentHeight = m_flickable.contentHeight.read().toReal();
        pageWidth     = m_flickable.width.read().toReal() - m_flickable.leftMargin.read().toReal() -
                    m_flickable.rightMargin.read().toReal();
        pageHeight = m_flickable.height.read().toReal() - m_flickable.topMargin.read().toReal() -
                     m_flickable.bottomMargin.read().toReal();
    }

    // The code handling touch, mouse and hover events is mostly copied/adapted from
    // QQuickScrollView::childMouseEventFilter()
    switch (event->type()) {
    case QEvent::Wheel: {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        // Can't use wheelEvent->deviceType() to determine device type since on Wayland mouse is
        // always regarded as touchpad
        // https://invent.kde.org/qt/qt/qtwayland/-/blob/e695a39519a7629c1549275a148cfb9ab99a07a9/src/client/qwaylandinputdevice.cpp#L445
        // and we can only expect a touchpad never generates the same angle delta as a mouse
        // m_wasTouched = std::abs(wheelEvent->angleDelta().y()) != 120 &&
        // std::abs(wheelEvent->angleDelta().x()) != 120;
        m_wasTouched = false;

        // NOTE: On X11 with libinput, pixelDelta is identical to angleDelta when using a mouse
        // that shouldn't use pixelDelta. If faulty pixelDelta, reset pixelDelta to (0,0).
        if (wheelEvent->pixelDelta() == wheelEvent->angleDelta()) {
            // In order to change any of the data, we have to create a whole new QWheelEvent
            // from its constructor.
            QWheelEvent newWheelEvent(wheelEvent->position(),
                                      wheelEvent->globalPosition(),
                                      QPoint(0, 0), // pixelDelta
                                      wheelEvent->angleDelta(),
                                      wheelEvent->buttons(),
                                      wheelEvent->modifiers(),
                                      wheelEvent->phase(),
                                      wheelEvent->inverted(),
                                      wheelEvent->source());
            m_kirigamiWheelEvent.initializeFromEvent(&newWheelEvent);
        } else {
            m_kirigamiWheelEvent.initializeFromEvent(wheelEvent);
        }

        Q_EMIT wheel(&m_kirigamiWheelEvent);

        if (m_kirigamiWheelEvent.isAccepted()) {
            return true;
        }

        bool scrolled = false;
        if (m_scrollFlickableTarget || (contentHeight <= pageHeight && contentWidth <= pageWidth)) {
            // Don't use pixelDelta from the event unless angleDelta is not available
            // because scrolling by pixelDelta is too slow on Wayland with libinput.
            QPointF pixelDelta = m_kirigamiWheelEvent.angleDelta().isNull()
                                     ? m_kirigamiWheelEvent.pixelDelta()
                                     : QPoint(0, 0);
            scrolled           = scrollFlickable(pixelDelta,
                                       m_kirigamiWheelEvent.angleDelta(),
                                       Qt::KeyboardModifiers(m_kirigamiWheelEvent.modifiers()));
        }
        setScrolling(scrolled);

        // NOTE: Wheel events created by touchpad gestures with pixel deltas will cause
        // scrolling to jump back to where scrolling started unless the event is always accepted
        // before it reaches the Flickable.
        bool flickableWillUseGestureScrolling =
            ! (wheelEvent->source() == Qt::MouseEventNotSynthesized ||
               wheelEvent->pixelDelta().isNull());

        if (scrolled) {
            Q_EMIT wheelMoved();
        }

        return scrolled || m_blockTargetWheel || flickableWillUseGestureScrolling;
    }

    case QEvent::TouchBegin: {
        m_wasTouched = true;
        break;
    }

    case QEvent::TouchEnd: {
        m_wasTouched = false;
        break;
    }

    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        setScrolling(false);
        break;
    }

    case QEvent::KeyPress: {
        if (! m_keyNavigationEnabled) {
            break;
        }
        QKeyEvent* keyEvent         = static_cast<QKeyEvent*>(event);
        bool       horizontalScroll = keyEvent->modifiers() & m_defaultHorizontalScrollModifiers;
        switch (keyEvent->key()) {
        case Qt::Key_Up: return scrollUp();
        case Qt::Key_Down: return scrollDown();
        case Qt::Key_Left: return scrollLeft();
        case Qt::Key_Right: return scrollRight();
        case Qt::Key_PageUp: return horizontalScroll ? scrollLeft(pageWidth) : scrollUp(pageHeight);
        case Qt::Key_PageDown:
            return horizontalScroll ? scrollRight(pageWidth) : scrollDown(pageHeight);
        case Qt::Key_Home:
            return horizontalScroll ? scrollLeft(contentWidth) : scrollUp(contentHeight);
        case Qt::Key_End:
            return horizontalScroll ? scrollRight(contentWidth) : scrollDown(contentHeight);
        default: break;
        }
        break;
    }

    default: break;
    }

    return false;
}

bool WheelHandler::useAnimation() const { return m_useAnimation; }

void WheelHandler::setUseAnimation(bool v) {
    if (std::exchange(m_useAnimation, v) != v) {
        Q_EMIT useAnimationChanged();
    }
}

void WheelHandler::rebindScrollBarV() { rebindScrollBar(m_scrollBarV); }
void WheelHandler::rebindScrollBarH() { rebindScrollBar(m_scrollBarH); }

#include "moc_wheelhandler.cpp"