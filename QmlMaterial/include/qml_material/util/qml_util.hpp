#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWindow>

#include "qml_material/util/corner.hpp"
#include "qml_material/enum.hpp"
#include "qml_material/token/token.hpp"
#include "qml_material/core.hpp"

namespace qml_material
{

class Util : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(UtilCpp)
public:
    Util(QObject* parent = nullptr);
    ~Util();

    enum Track
    {
        TrackCreate = 0,
        TrackDelete
    };
    Q_ENUMS(Track)

    Q_INVOKABLE void track(QVariant, Track);

    Q_INVOKABLE bool hasIcon(const QJSValue& v) const;

    Q_INVOKABLE static void closePopup(QObject* obj);

    Q_INVOKABLE static QColor transparent(QColor in, float alpha) noexcept;
    Q_INVOKABLE static QColor hoverColor(QColor in) noexcept;
    Q_INVOKABLE static QColor pressColor(QColor in) noexcept;

    Q_INVOKABLE static qreal devicePixelRatio(QQuickItem* in);

    Q_INVOKABLE static CornersGroup listCorners(qint32 idx, qint32 count, qint32 radius) noexcept;
    Q_INVOKABLE static CornersGroup tableCorners(qint32 row, qint32 column, qint32 rows,
                                                 qint32 columns, qint32 radius) noexcept;
    Q_INVOKABLE static CornersGroup tableWithHeaderCorners(qint32 row, qint32 column, qint32 rows,
                                                           qint32 columns, qint32 radius) noexcept;

    Q_INVOKABLE static CornersGroup cornerArray(QVariant in) noexcept;
    Q_INVOKABLE static CornersGroup corners(qreal in) noexcept;
    Q_INVOKABLE static CornersGroup corners(qreal, qreal) noexcept;
    Q_INVOKABLE static CornersGroup corners(qreal tl, qreal tr, qreal bl, qreal br) noexcept;

    QString type_str(const QJSValue&);

    Q_INVOKABLE void print_parents(const QJSValue&);

    Q_INVOKABLE static qreal lightness(QColor color) noexcept;

    Q_INVOKABLE static token::Elevation tokenElevation() noexcept;
    Q_INVOKABLE static token::Shape     tokenShape() noexcept;
    Q_INVOKABLE static token::State     tokenState() noexcept;

    Q_INVOKABLE QObject* createItem(const QJSValue& url_or_comp, const QVariantMap& props,
                                    QObject* parent = nullptr);

    Q_INVOKABLE QObject* showPopup(const QJSValue& url_or_comp, const QVariantMap& props,
                                   QObject* parent = nullptr, bool open_and_destry = true);

    Q_INVOKABLE static QString paramsString(const QVariantMap& props);
    Q_INVOKABLE static void    setCursor(QQuickItem* item, Qt::CursorShape shape);

    Q_INVOKABLE static double clamp(double t, double low, double high);
    Q_INVOKABLE static double teleportCurve(double t, double left, double right);

    Q_INVOKABLE static void forceSetImplicitHeight(QQuickItem* item, qreal height);
    Q_INVOKABLE static void forceSetImplicitWidth(QQuickItem* item, qreal width);

    Q_INVOKABLE static void     cellHoveredOn(QQuickItem* item, bool hovered, qint32 row,
                                              qint32 column);
    Q_INVOKABLE static QObject* getParent(QObject* obj);
    Q_INVOKABLE static bool     disconnectAll(QObject* obj, const QString&);

private:
    Q_SLOT void onPopupClosed();

private:
    usize m_tracked { 0 };
};

auto tryCreateComponent(const QVariant& val, QQmlComponent::CompilationMode useAsync,
                        const std::function<QQmlComponent*()>& createComponent) -> QQmlComponent*;
} // namespace qml_material

namespace qcm
{
auto qml_dyn_count() -> std::atomic<i32>&;
auto createItem(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                QObject* parent) -> QObject*;

} // namespace qcm
