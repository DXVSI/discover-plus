#pragma once

#include <QQmlEngine>
#include <QQuickItem>

namespace qml_material
{

class InputBlock : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged FINAL)
    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(Qt::MouseButtons acceptMouseButtons READ acceptMouseButtons WRITE
                   setAcceptMouseButtons NOTIFY acceptMouseButtonsChanged FINAL)
    Q_PROPERTY(bool acceptHover READ acceptHoverEvents WRITE setAcceptHoverEvents NOTIFY
                   acceptHoverEventsChanged FINAL)
    Q_PROPERTY(bool acceptTouch READ acceptTouchEvents WRITE setAcceptTouchEvents NOTIFY
                   acceptTouchEventsChanged FINAL)
    Q_PROPERTY(bool acceptWheel READ acceptWheelEvents WRITE setAcceptWheelEvents NOTIFY
                   acceptWheelEventsChanged FINAL)

public:
    InputBlock(QObject* parent = nullptr);

    QQuickItem* target() const;
    void        setTarget(QQuickItem*);

    bool when() const;
    void setWhen(bool);

    Qt::MouseButtons acceptMouseButtons() const;
    void             setAcceptMouseButtons(Qt::MouseButtons buttons);
    bool             acceptHoverEvents() const;
    void             setAcceptHoverEvents(bool enabled);
    bool             acceptTouchEvents() const;
    void             setAcceptTouchEvents(bool accept);
    bool             acceptWheelEvents() const;
    void             setAcceptWheelEvents(bool accept);

    bool eventFilter(QObject* obj, QEvent* event) override;

    Q_SIGNAL void whenChanged();
    Q_SIGNAL void targetChanged();
    Q_SIGNAL void acceptMouseButtonsChanged();
    Q_SIGNAL void acceptHoverEventsChanged();
    Q_SIGNAL void acceptTouchEventsChanged();
    Q_SIGNAL void acceptWheelEventsChanged();

    Q_SLOT void trigger();

private:
    struct State {
        bool             canHover { false };
        bool             canTouch { false };
        Qt::MouseButtons mouseButtons { Qt::NoButton };
        void             saveState(QQuickItem*);
        void             restoreState(QQuickItem*);
    };

private:
    bool                 mWhen;
    bool                 mAcceptWheel;
    QPointer<QQuickItem> mTarget;
    State                mState;
    State                mReqState;
};

} // namespace qml_material