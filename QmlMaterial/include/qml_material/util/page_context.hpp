#pragma once

#include <QtQml/QQmlEngine>

namespace qml_material
{
class PageContext : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(PageContext* inherit READ inherit WRITE setInherit NOTIFY inheritChanged FINAL)
    Q_PROPERTY(QObject* leadingAction READ leadingAction WRITE setLeadingAction NOTIFY
                   leadingActionChanged FINAL)
    Q_PROPERTY(qint32 headerType READ headerType WRITE setHeaderType NOTIFY headerTypeChanged FINAL)
    Q_PROPERTY(qint32 backgroundRadius READ backgroundRadius WRITE setBackgroundRadius NOTIFY
                   backgroundRadiusChanged FINAL)
    Q_PROPERTY(double headerBackgroundOpacity READ headerBackgroundOpacity WRITE
                   setHeaderBackgroundOpacity NOTIFY headerBackgroundOpacityChanged FINAL)
    Q_PROPERTY(qint32 radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(bool showHeader READ showHeader WRITE setShowHeader NOTIFY showHeaderChanged FINAL)
    Q_PROPERTY(bool showBackground READ showBackground WRITE setShowBackground NOTIFY
                   showBackgroundChanged FINAL)
public:
    PageContext(QObject* parent = nullptr);
    ~PageContext();

    auto inherit() const -> PageContext*;
    auto leadingAction() const -> QObject*;
    auto backgroundRadius() const -> qint32;
    auto headerBackgroundOpacity() const -> double;
    auto radius() const -> qint32;
    auto headerType() const -> qint32;
    auto showHeader() const -> bool;
    auto showBackground() const -> bool;

    Q_SLOT void setInherit(PageContext*);
    Q_SLOT void setLeadingAction(QObject*);
    Q_SLOT void setBackgroundRadius(qint32);
    Q_SLOT void setHeaderBackgroundOpacity(double);
    Q_SLOT void setRadius(qint32);
    Q_SLOT void setHeaderType(qint32);
    Q_SLOT void setShowHeader(bool);
    Q_SLOT void setShowBackground(bool);

    Q_SIGNAL void inheritChanged();
    Q_SIGNAL void leadingActionChanged();
    Q_SIGNAL void backgroundRadiusChanged();
    Q_SIGNAL void headerBackgroundOpacityChanged();
    Q_SIGNAL void radiusChanged();
    Q_SIGNAL void headerTypeChanged();
    Q_SIGNAL void showHeaderChanged();
    Q_SIGNAL void showBackgroundChanged();

    Q_SIGNAL void pushItem(const QJSValue& url_or_comp, const QVariantMap& props = {});
    Q_SIGNAL void pop();

private:
    PageContext* m_inherit;

    std::optional<QObject*> m_leading_action;
    std::optional<qint32>   m_header_type;
    std::optional<qint32>   m_background_radius;
    std::optional<double>   m_header_background_opacity;
    std::optional<qint32>   m_radius;
    std::optional<bool>     m_show_header;
    std::optional<bool>     m_show_background;
};
} // namespace qml_material