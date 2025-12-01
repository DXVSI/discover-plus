#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQml/QQmlEngine>
#include <QtGui/QColor>
#include <QtQuickControls2/QQuickAttachedPropertyPropagator>

#include "qml_material/token/color.hpp"
#include "qml_material/util/page_context.hpp"

#define ATTACH_PROPERTY(Type, Name)                                                                \
private:                                                                                           \
    Q_PROPERTY(Type Name READ Name WRITE set_##Name RESET reset_##Name NOTIFY Name##Changed FINAL) \
public:                                                                                            \
    Type              Name() const;                                                                \
    void              set_##Name(Type);                                                            \
    void              reset_##Name();                                                              \
    AttachProp<Type>& get_##Name();                                                                \
    Q_SIGNAL void     Name##Changed();                                                             \
                                                                                                   \
private:                                                                                           \
    AttachProp<Type> m_##Name { &Self::Name##Changed };

namespace qml_material
{

class ThemeSize : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    ThemeSize(QObject* parent = nullptr);
    ~ThemeSize();
    Q_PROPERTY(
        qint32 windowClass READ windowClass WRITE setWindowClass NOTIFY windowClassChanged FINAL)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY windowClassChanged FINAL)
    Q_PROPERTY(bool isMedium READ isMedium NOTIFY windowClassChanged FINAL)
    Q_PROPERTY(bool isExpanded READ isExpanded NOTIFY windowClassChanged FINAL)
    Q_PROPERTY(bool isLarge READ isLarge NOTIFY windowClassChanged FINAL)
    Q_PROPERTY(bool isExtraLarge READ isExtraLarge NOTIFY windowClassChanged FINAL)

    Q_PROPERTY(qint32 duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(qint32 width READ width WRITE setWidth NOTIFY widthChanged FINAL)

    auto isCompact() const -> bool;
    auto isMedium() const -> bool;
    auto isExpanded() const -> bool;
    auto isLarge() const -> bool;
    auto isExtraLarge() const -> bool;

    auto          width() const -> qint32;
    void          setWidth(qint32 w);
    Q_SIGNAL void widthChanged(qint32);

    auto          duration() const -> qint32;
    void          setDuration(qint32 w);
    Q_SIGNAL void durationChanged(qint32);

    auto          windowClass() const -> qint32;
    void          setWindowClass(qint32);
    Q_SIGNAL void windowClassChanged(qint32);

    Q_PROPERTY(qint32 verticalPadding READ verticalPadding NOTIFY verticalPaddingChanged FINAL)
    auto          verticalPadding() const -> qint32;
    Q_SIGNAL void verticalPaddingChanged();

private:
    qint32 m_window_class;
    qint32 m_width;
    qint32 m_duration;
    QTimer  m_width_timer;
};

class Theme : public QQuickAttachedPropertyPropagator {
    Q_OBJECT

    QML_NAMED_ELEMENT(MProp)
    QML_UNCREATABLE("")
    QML_ATTACHED(Theme)

public:
    using Self = Theme;

    template<typename V>
    struct AttachProp {
        using SigFunc   = void (Theme::*)();
        using ReadFunc  = V (Theme::* const)();
        using WriteFunc = void (Theme::*)(V);
        using GetFunc   = AttachProp<V>& (Theme::* const)();

        std::optional<V> value;
        bool             explicited;
        SigFunc          sig_func;

        AttachProp(SigFunc s): value(), explicited(false), sig_func(s) {}
    };

    ATTACH_PROPERTY(QColor, textColor)
    ATTACH_PROPERTY(QColor, backgroundColor)
    ATTACH_PROPERTY(int, elevation)
    ATTACH_PROPERTY(qml_material::MdColorMgr*, color)
    ATTACH_PROPERTY(qml_material::ThemeSize*, size)
    ATTACH_PROPERTY(qml_material::PageContext*, page)

public:
    Theme(QObject* parent);
    ~Theme();

    static Theme* qmlAttachedProperties(QObject* object);

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator* newParent,
                              QQuickAttachedPropertyPropagator* oldParent) override;
};
} // namespace qml_material

#undef ATTACH_PROPERTY