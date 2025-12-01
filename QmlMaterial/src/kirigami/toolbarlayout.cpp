/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "toolbarlayout.h"

#include <cmath>
#include <unordered_map>

#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QQmlComponent>
#include <QTimer>

#include "util/loggingcategory.hpp"
#include "toolbarlayoutdelegate.h"

ToolBarLayoutAttached::ToolBarLayoutAttached(QObject* parent): QObject(parent) {}

QObject* ToolBarLayoutAttached::action() const { return m_action; }

void ToolBarLayoutAttached::setAction(QObject* action) { m_action = action; }

class ToolBarLayoutPrivate {
    ToolBarLayout* const q;

public:
    ToolBarLayoutPrivate(ToolBarLayout* qq): q(qq) {}
    ~ToolBarLayoutPrivate() {
        if (moreButtonIncubator) {
            moreButtonIncubator->clear();
            delete moreButtonIncubator;
        }
    }

    void                          calculateImplicitSize();
    void                          performLayout();
    QList<ToolBarLayoutDelegate*> createDelegates();
    ToolBarLayoutDelegate*        createDelegate(QObject* action);
    qreal                         layoutStart(qreal layoutWidth);
    void maybeHideDelegate(int index, qreal& currentWidth, qreal totalWidth);

    QList<QObject*>                actions;
    ToolBarLayout::ActionsProperty actionsProperty;
    QList<QObject*>                hiddenActions;
    QQmlComponent*                 fullDelegate        = nullptr;
    QQmlComponent*                 iconDelegate        = nullptr;
    QQmlComponent*                 separatorDelegate   = nullptr;
    QQmlComponent*                 moreButton          = nullptr;
    qreal                          spacing             = 0.0;
    Qt::Alignment                  alignment           = Qt::AlignLeft;
    qreal                          visibleActionsWidth = 0.0;
    qreal                          visibleWidth        = 0.0;
    Qt::LayoutDirection            layoutDirection     = Qt::LeftToRight;
    ToolBarLayout::HeightMode      heightMode          = ToolBarLayout::ConstrainIfLarger;
    qint32                         maxShowActionNum    = std::numeric_limits<qint32>::max();

    bool completed         = false;
    bool actionsChanged    = false;
    bool implicitSizeValid = false;

    std::unordered_map<QObject*, std::unique_ptr<ToolBarLayoutDelegate>> delegates;
    QList<ToolBarLayoutDelegate*>                                        sortedDelegates;
    QQuickItem*               moreButtonInstance   = nullptr;
    ToolBarDelegateIncubator* moreButtonIncubator  = nullptr;
    bool                      shouldShowMoreButton = false;
    int                       firstHiddenIndex     = -1;

    QList<QObject*> removedActions;
    QTimer*         removalTimer = nullptr;

    QElapsedTimer performanceTimer;

    static void      appendAction(ToolBarLayout::ActionsProperty* list, QObject* action);
    static qsizetype actionCount(ToolBarLayout::ActionsProperty* list);
    static QObject*  action(ToolBarLayout::ActionsProperty* list, qsizetype index);
    static void      clearActions(ToolBarLayout::ActionsProperty* list);
};

ToolBarLayout::ToolBarLayout(QQuickItem* parent)
    : QQuickItem(parent), d(std::make_unique<ToolBarLayoutPrivate>(this)) {
    d->actionsProperty = ActionsProperty(this,
                                         this,
                                         ToolBarLayoutPrivate::appendAction,
                                         ToolBarLayoutPrivate::actionCount,
                                         ToolBarLayoutPrivate::action,
                                         ToolBarLayoutPrivate::clearActions);

    // To prevent multiple assignments to actions from constantly recreating
    // delegates, we cache the delegates and only remove them once they are no
    // longer being used. This timer is responsible for triggering that removal.
    d->removalTimer = new QTimer { this };
    d->removalTimer->setInterval(1000);
    d->removalTimer->setSingleShot(true);
    connect(d->removalTimer, &QTimer::timeout, this, [this]() {
        for (auto action : std::as_const(d->removedActions)) {
            if (! d->actions.contains(action)) {
                d->delegates.erase(action);
            }
        }
        d->removedActions.clear();
    });
}

ToolBarLayout::~ToolBarLayout() {}

ToolBarLayout::ActionsProperty ToolBarLayout::actionsProperty() const { return d->actionsProperty; }

void ToolBarLayout::addAction(QObject* action) {
    if (action == nullptr) {
        return;
    }
    d->actions.append(action);
    d->actionsChanged = true;

    connect(action, &QObject::destroyed, this, [this](QObject* action) {
        auto itr = d->delegates.find(action);
        if (itr != d->delegates.end()) {
            d->delegates.erase(itr);
        }

        d->actions.removeOne(action);
        d->actionsChanged = true;

        relayout();
    });

    relayout();
}

void ToolBarLayout::removeAction(QObject* action) {
    auto itr = d->delegates.find(action);
    if (itr != d->delegates.end()) {
        itr->second->hide();
    }

    d->actions.removeOne(action);
    d->removedActions.append(action);
    d->removalTimer->start();
    d->actionsChanged = true;

    relayout();
}

void ToolBarLayout::clearActions() {
    for (auto action : std::as_const(d->actions)) {
        auto itr = d->delegates.find(action);
        if (itr != d->delegates.end()) {
            itr->second->hide();
        }
    }

    d->removedActions.append(d->actions);
    d->actions.clear();
    d->actionsChanged = true;

    relayout();
}

QList<QObject*> ToolBarLayout::hiddenActions() const { return d->hiddenActions; }

QQmlComponent* ToolBarLayout::fullDelegate() const { return d->fullDelegate; }

void ToolBarLayout::setFullDelegate(QQmlComponent* newFullDelegate) {
    if (newFullDelegate == d->fullDelegate) {
        return;
    }

    d->fullDelegate = newFullDelegate;
    d->delegates.clear();
    relayout();
    Q_EMIT fullDelegateChanged();
}

QQmlComponent* ToolBarLayout::iconDelegate() const { return d->iconDelegate; }

void ToolBarLayout::setIconDelegate(QQmlComponent* newIconDelegate) {
    if (newIconDelegate == d->iconDelegate) {
        return;
    }

    d->iconDelegate = newIconDelegate;
    d->delegates.clear();
    relayout();
    Q_EMIT iconDelegateChanged();
}

QQmlComponent* ToolBarLayout::separatorDelegate() const { return d->separatorDelegate; }

void ToolBarLayout::setSeparatorDelegate(QQmlComponent* newSeparatorDelegate) {
    if (newSeparatorDelegate == d->separatorDelegate) {
        return;
    }

    d->separatorDelegate = newSeparatorDelegate;
    d->delegates.clear();
    relayout();
    Q_EMIT separatorDelegateChanged();
}

QQmlComponent* ToolBarLayout::moreButton() const { return d->moreButton; }

void ToolBarLayout::setMoreButton(QQmlComponent* newMoreButton) {
    if (newMoreButton == d->moreButton) {
        return;
    }

    d->moreButton = newMoreButton;
    if (d->moreButtonInstance) {
        d->moreButtonInstance->deleteLater();
        d->moreButtonInstance = nullptr;
    }
    relayout();
    Q_EMIT moreButtonChanged();
}

qreal ToolBarLayout::spacing() const { return d->spacing; }

void ToolBarLayout::setSpacing(qreal newSpacing) {
    if (newSpacing == d->spacing) {
        return;
    }

    d->spacing = newSpacing;
    relayout();
    Q_EMIT spacingChanged();
}

Qt::Alignment ToolBarLayout::alignment() const { return d->alignment; }

void ToolBarLayout::setAlignment(Qt::Alignment newAlignment) {
    if (newAlignment == d->alignment) {
        return;
    }

    d->alignment = newAlignment;
    relayout();
    Q_EMIT alignmentChanged();
}

qreal ToolBarLayout::visibleWidth() const { return d->visibleWidth; }

qreal ToolBarLayout::minimumWidth() const {
    return d->moreButtonInstance ? d->moreButtonInstance->width() : 0;
}

Qt::LayoutDirection ToolBarLayout::layoutDirection() const { return d->layoutDirection; }

void ToolBarLayout::setLayoutDirection(Qt::LayoutDirection& newLayoutDirection) {
    if (newLayoutDirection == d->layoutDirection) {
        return;
    }

    d->layoutDirection = newLayoutDirection;
    relayout();
    Q_EMIT layoutDirectionChanged();
}

ToolBarLayout::HeightMode ToolBarLayout::heightMode() const { return d->heightMode; }

void ToolBarLayout::setHeightMode(HeightMode newHeightMode) {
    if (newHeightMode == d->heightMode) {
        return;
    }

    d->heightMode = newHeightMode;
    relayout();
    Q_EMIT heightModeChanged();
}

qint32 ToolBarLayout::maxShowActionNum() const { return d->maxShowActionNum; }
void   ToolBarLayout::setMaxShowActionNum(qint32 maxShowActionNum) {
    if (maxShowActionNum == d->maxShowActionNum) {
        return;
    }
    d->maxShowActionNum = maxShowActionNum;
    maxShowActionNumChanged();
}

void ToolBarLayout::relayout() {
    d->implicitSizeValid = false;
    polish();
}

void ToolBarLayout::componentComplete() {
    QQuickItem::componentComplete();
    d->completed = true;
    relayout();
}

void ToolBarLayout::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    if (newGeometry != oldGeometry) {
        if (newGeometry.size() != QSizeF { implicitWidth(), implicitHeight() }) {
            relayout();
        } else {
            polish();
        }
    }
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void ToolBarLayout::itemChange(QQuickItem::ItemChange            change,
                               const QQuickItem::ItemChangeData& data) {
    if (change == ItemVisibleHasChanged || change == ItemSceneChange) {
        relayout();
    }
    QQuickItem::itemChange(change, data);
}

void ToolBarLayout::updatePolish() { d->performLayout(); }

/**
 * Calculate the implicit size for this layout.
 *
 * This is a separate step from performing the actual layout, because of a nasty
 * little issue with Control, where it will unconditionally set the height of
 * its contentItem, which means QQuickItem::heightValid() becomes useless. So
 * instead, we first calculate our implicit size, ignoring any explicitly set
 * item size. Then we follow that by performing the actual layouting, using the
 * width and height retrieved from the item, as those will return the explicitly
 * set width/height if set and the implicit size otherwise. Since control
 * watches for implicit size changes, we end up with correct behaviour both when
 * we get an explicit size set and when we're relying on implicit size
 * calculation.
 */
void ToolBarLayoutPrivate::calculateImplicitSize() {
    if (! completed) {
        return;
    }

    if (! fullDelegate || ! iconDelegate || ! separatorDelegate || ! moreButton) {
        qCWarning(qml_material_logcat())
            << "ToolBarLayout: Unable to layout, required properties are not set";
        return;
    }

    if (actions.isEmpty()) {
        q->setImplicitSize(0., 0.);
        return;
    }

    hiddenActions.clear();
    firstHiddenIndex = -1;

    sortedDelegates = createDelegates();

    bool ready = std::all_of(
        delegates.cbegin(),
        delegates.cend(),
        [](const std::pair<QObject* const, std::unique_ptr<ToolBarLayoutDelegate>>& entry) {
            return entry.second->isReady();
        });
    if (! ready || ! moreButtonInstance) {
        return;
    }

    qreal maxHeight = 0.0;
    qreal maxWidth  = 0.0;

    // First, calculate the total width and maximum height of all delegates.
    // This will be used to determine which actions to show, which ones to
    // collapse to icon-only etc.
    for (auto entry : std::as_const(sortedDelegates)) {
        if (! entry->isActionVisible()) {
            entry->hide();
            continue;
        }

        if (entry->isHidden()) {
            entry->hide();
            hiddenActions.append(entry->action());
            continue;
        }

        if (entry->isIconOnly()) {
            entry->showIcon();
        } else {
            entry->showFull();
        }

        maxWidth += entry->width() + spacing;
        maxHeight = std::max(maxHeight, entry->maxHeight());
    }

    // The last entry also gets spacing but shouldn't, so remove that.
    maxWidth -= spacing;

    visibleActionsWidth = 0.0;

    if (maxWidth >
        q->width() - (hiddenActions.isEmpty() ? 0.0 : moreButtonInstance->width() + spacing)) {
        // We have more items than fit into the view, so start hiding some.

        qreal layoutWidth = q->width() - (moreButtonInstance->width() + spacing);
        if (alignment & Qt::AlignHCenter) {
            // When centering, we need to reserve space on both sides to make sure
            // things are properly centered, otherwise we will be to the right of
            // the center.
            layoutWidth -= (moreButtonInstance->width() + spacing);
        }

        for (int i = 0; i < sortedDelegates.size(); ++i) {
            auto delegate = sortedDelegates.at(i);

            maybeHideDelegate(i, visibleActionsWidth, layoutWidth);

            if (delegate->isVisible()) {
                visibleActionsWidth += delegate->width() + spacing;
            }
        }
        if (! qFuzzyIsNull(visibleActionsWidth)) {
            // Like above, remove spacing on the last element that incorrectly gets spacing added.
            visibleActionsWidth -= spacing;
        }
    } else {
        visibleActionsWidth = maxWidth;
    }

    if (sortedDelegates.size() > q->maxShowActionNum() + hiddenActions.size()) {
        // hide if raise on limit
        for (auto i = q->maxShowActionNum(); i < sortedDelegates.size(); i++) {
            auto* delegate = sortedDelegates[i];
            delegate->hide();
            hiddenActions.append(delegate->action());
        }
    }

    if (! hiddenActions.isEmpty()) {
        maxHeight = std::max(maxHeight, moreButtonInstance->implicitHeight());
    };

    q->setImplicitSize(maxWidth, maxHeight);
    Q_EMIT q->hiddenActionsChanged();

    implicitSizeValid = true;

    q->polish();
}

void ToolBarLayoutPrivate::performLayout() {
    if (! completed || actions.isEmpty()) {
        return;
    }

    if (! implicitSizeValid) {
        calculateImplicitSize();
    }

    if (sortedDelegates.isEmpty()) {
        sortedDelegates = createDelegates();
    }

    bool ready = std::all_of(
        delegates.cbegin(),
        delegates.cend(),
        [](const std::pair<QObject* const, std::unique_ptr<ToolBarLayoutDelegate>>& entry) {
            return entry.second->isReady();
        });
    if (! ready || ! moreButtonInstance) {
        return;
    }

    qreal width  = q->width();
    qreal height = q->height();

    if (! hiddenActions.isEmpty()) {
        if (layoutDirection == Qt::LeftToRight) {
            moreButtonInstance->setX(width - moreButtonInstance->width());
        } else {
            moreButtonInstance->setX(0.0);
        }

        if (heightMode == ToolBarLayout::AlwaysFill) {
            moreButtonInstance->setHeight(height);
        } else if (heightMode == ToolBarLayout::ConstrainIfLarger) {
            if (moreButtonInstance->implicitHeight() > height) {
                moreButtonInstance->setHeight(height);
            } else {
                moreButtonInstance->resetHeight();
            }
        } else {
            moreButtonInstance->resetHeight();
        }

        moreButtonInstance->setY(qRound((height - moreButtonInstance->height()) / 2.0));
        shouldShowMoreButton = true;
        moreButtonInstance->setVisible(true);
    } else {
        shouldShowMoreButton = false;
        moreButtonInstance->setVisible(false);
    }

    qreal currentX = layoutStart(visibleActionsWidth);
    for (auto entry : std::as_const(sortedDelegates)) {
        if (! entry->isVisible()) {
            continue;
        }

        if (heightMode == ToolBarLayout::AlwaysFill) {
            entry->setHeight(height);
        } else if (heightMode == ToolBarLayout::ConstrainIfLarger) {
            if (entry->implicitHeight() > height) {
                entry->setHeight(height);
            } else {
                entry->resetHeight();
            }
        } else {
            entry->resetHeight();
        }

        qreal y = qRound((height - entry->height()) / 2.0);

        if (layoutDirection == Qt::LeftToRight) {
            entry->setPosition(currentX, y);
            currentX += entry->width() + spacing;
        } else {
            entry->setPosition(currentX - entry->width(), y);
            currentX -= entry->width() + spacing;
        }

        entry->show();
    }

    qreal newVisibleWidth = visibleActionsWidth;
    if (moreButtonInstance->isVisible()) {
        newVisibleWidth += moreButtonInstance->width() + (newVisibleWidth > 0.0 ? spacing : 0.0);
    }
    if (! qFuzzyCompare(newVisibleWidth, visibleWidth)) {
        visibleWidth = newVisibleWidth;
        Q_EMIT q->visibleWidthChanged();
    }

    if (actionsChanged) {
        // Due to the way QQmlListProperty works, if we emit changed every time
        // an action is added/removed, we end up emitting way too often. So
        // instead only do it after everything else is done.
        Q_EMIT q->actionsChanged();
        actionsChanged = false;
    }

    sortedDelegates.clear();
}

QList<ToolBarLayoutDelegate*> ToolBarLayoutPrivate::createDelegates() {
    QList<ToolBarLayoutDelegate*> result;
    for (auto action : std::as_const(actions)) {
        if (delegates.find(action) != delegates.end()) {
            result.append(delegates.at(action).get());
        } else if (action) {
            auto delegate = std::unique_ptr<ToolBarLayoutDelegate>(createDelegate(action));
            if (delegate) {
                result.append(delegate.get());
                delegates.emplace(action, std::move(delegate));
            }
        }
    }

    if (! moreButtonInstance && ! moreButtonIncubator) {
        moreButtonIncubator = new ToolBarDelegateIncubator(moreButton, qmlContext(moreButton));
        moreButtonIncubator->setStateCallback([this](QQuickItem* item) {
            item->setParentItem(q);
        });
        moreButtonIncubator->setCompletedCallback([this](ToolBarDelegateIncubator* incubator) {
            moreButtonInstance = qobject_cast<QQuickItem*>(incubator->object());
            moreButtonInstance->setVisible(false);

            QObject::connect(moreButtonInstance, &QQuickItem::visibleChanged, q, [this]() {
                moreButtonInstance->setVisible(shouldShowMoreButton);
            });
            QObject::connect(moreButtonInstance,
                             &QQuickItem::widthChanged,
                             q,
                             &ToolBarLayout::minimumWidthChanged);
            q->relayout();
            Q_EMIT q->minimumWidthChanged();

            QTimer::singleShot(0, q, [this]() {
                delete moreButtonIncubator;
                moreButtonIncubator = nullptr;
            });
        });
        moreButtonIncubator->create();
    }

    return result;
}

ToolBarLayoutDelegate* ToolBarLayoutPrivate::createDelegate(QObject* action) {
    QQmlComponent* fullComponent    = nullptr;
    auto           displayComponent = action->property("displayComponent");
    if (displayComponent.isValid()) {
        fullComponent = displayComponent.value<QQmlComponent*>();
    }

    if (! fullComponent) {
        fullComponent = fullDelegate;
    }

    auto separator = action->property("separator");
    if (separator.isValid() && separator.toBool()) {
        fullComponent = separatorDelegate;
    }

    auto result = new ToolBarLayoutDelegate(q);
    result->setAction(action);
    result->createItems(fullComponent, iconDelegate, [this, action](QQuickItem* newItem) {
        newItem->setParentItem(q);
        auto attached = static_cast<ToolBarLayoutAttached*>(
            qmlAttachedPropertiesObject<ToolBarLayout>(newItem, true));
        attached->setAction(action);
    });

    return result;
}

qreal ToolBarLayoutPrivate::layoutStart(qreal layoutWidth) {
    qreal availableWidth = moreButtonInstance->isVisible()
                               ? q->width() - (moreButtonInstance->width() + spacing)
                               : q->width();

    if (alignment & Qt::AlignLeft) {
        return layoutDirection == Qt::LeftToRight ? 0.0 : q->width();
    } else if (alignment & Qt::AlignHCenter) {
        return (q->width() / 2) +
               (layoutDirection == Qt::LeftToRight ? -layoutWidth / 2.0 : layoutWidth / 2.0);
    } else if (alignment & Qt::AlignRight) {
        qreal offset = availableWidth - layoutWidth;
        return layoutDirection == Qt::LeftToRight ? offset : q->width() - offset;
    }
    return 0.0;
}

void ToolBarLayoutPrivate::maybeHideDelegate(int index, qreal& currentWidth, qreal totalWidth) {
    auto delegate = sortedDelegates.at(index);

    if (! delegate->isVisible()) {
        // If the delegate isn't visible anyway, do nothing.
        return;
    }

    if (currentWidth + delegate->width() < totalWidth &&
        (firstHiddenIndex < 0 || index < firstHiddenIndex)) {
        // If the delegate is fully visible and we have not already hidden
        // actions, do nothing.
        return;
    }

    if (delegate->isKeepVisible()) {
        // If the action is marked as KeepVisible, we need to try our best to
        // keep it in view. If the full size delegate does not fit, we try the
        // icon-only delegate. If that also does not fit, try and find other
        // actions to hide. Finally, if that also fails, we will hide the
        // delegate.
        if (currentWidth + delegate->iconWidth() > totalWidth) {
            // First, hide any earlier actions that are not marked as KeepVisible.
            for (auto currentIndex = index - 1; currentIndex >= 0; --currentIndex) {
                auto previousDelegate = sortedDelegates.at(currentIndex);
                if (! previousDelegate->isVisible() || previousDelegate->isKeepVisible()) {
                    continue;
                }

                auto width = previousDelegate->width();
                previousDelegate->hide();
                hiddenActions.append(previousDelegate->action());
                currentWidth -= (width + spacing);

                if (currentWidth + delegate->fullWidth() <= totalWidth) {
                    delegate->showFull();
                    break;
                } else if (currentWidth + delegate->iconWidth() <= totalWidth) {
                    delegate->showIcon();
                    break;
                }
            }

            if (currentWidth + delegate->width() <= totalWidth) {
                return;
            }

            // Hiding normal actions did not help enough, so go through actions
            // with KeepVisible set and try and collapse them to IconOnly.
            for (auto currentIndex = index - 1; currentIndex >= 0; --currentIndex) {
                auto previousDelegate = sortedDelegates.at(currentIndex);
                if (! previousDelegate->isVisible() || ! previousDelegate->isKeepVisible()) {
                    continue;
                }

                auto extraSpace = previousDelegate->width() - previousDelegate->iconWidth();
                previousDelegate->showIcon();
                currentWidth -= extraSpace;

                if (currentWidth + delegate->fullWidth() <= totalWidth) {
                    delegate->showFull();
                    break;
                } else if (currentWidth + delegate->iconWidth() <= totalWidth) {
                    delegate->showIcon();
                    break;
                }
            }

            // If that also did not work, then hide this action after all.
            if (currentWidth + delegate->width() > totalWidth) {
                delegate->hide();
                hiddenActions.append(delegate->action());
            }
        } else {
            delegate->showIcon();
        }
    } else {
        // The action is not marked as KeepVisible and it does not fit within
        // the current layout, so hide it.
        delegate->hide();
        hiddenActions.append(delegate->action());

        // If this is the first item to be hidden, mark it so we know we should
        // also hide the following items.
        if (firstHiddenIndex < 0) {
            firstHiddenIndex = index;
        }
    }
}

void ToolBarLayoutPrivate::appendAction(ToolBarLayout::ActionsProperty* list, QObject* action) {
    auto layout = reinterpret_cast<ToolBarLayout*>(list->data);
    layout->addAction(action);
}

qsizetype ToolBarLayoutPrivate::actionCount(ToolBarLayout::ActionsProperty* list) {
    return reinterpret_cast<ToolBarLayout*>(list->data)->d->actions.count();
}

QObject* ToolBarLayoutPrivate::action(ToolBarLayout::ActionsProperty* list, qsizetype index) {
    return reinterpret_cast<ToolBarLayout*>(list->data)->d->actions.at(index);
}

void ToolBarLayoutPrivate::clearActions(ToolBarLayout::ActionsProperty* list) {
    reinterpret_cast<ToolBarLayout*>(list->data)->clearActions();
}

#include "moc_toolbarlayout.cpp"
