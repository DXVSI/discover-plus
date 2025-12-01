#include "qml_material/util/qml_util.hpp"

#include <format>
#include <QQmlEngine>
#include <QJsonDocument>
#include <QJsonObject>

#include "qml_material/util/loggingcategory.hpp"

using namespace std::string_view_literals;

Q_LOGGING_CATEGORY(qml_material_logcat, "qcm.material", QtMsgType::QtInfoMsg)
namespace
{

// 2 time ease
auto easeInOut(double x) -> double { return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2; }

} // namespace

namespace qml_material
{

Util::Util(QObject* parent): QObject(parent) {}
Util::~Util() {}

bool Util::hasIcon(const QJSValue& v) const {
    auto name   = v.property(QStringLiteral("name"));
    auto source = v.property(QStringLiteral("source"));
    if (name.isString() && source.toVariant().isValid()) {
        return ! name.toString().isEmpty() || ! source.toString().isEmpty();
    }
    return false;
}

auto Util::transparent(QColor in, float alpha) noexcept -> QColor {
    in.setAlphaF(alpha);
    return in;
}

auto Util::hoverColor(QColor in) noexcept -> QColor {
    in.setAlphaF(0.08);
    return in;
}

auto Util::pressColor(QColor in) noexcept -> QColor {
    in.setAlphaF(0.18);
    return in;
}

void Util::closePopup(QObject* obj) {
    do {
        auto meta = obj->metaObject();
        do {
            auto cn = meta->className();
            if (cn == "QQuickPopup"sv) {
                QMetaObject::invokeMethod(obj, "close");
                return;
            }
        } while (meta = meta->superClass(), meta);
    } while (obj = obj->parent(), obj);
}

auto Util::devicePixelRatio(QQuickItem* in) -> qreal {
    return in ? in->window() ? in->window()->devicePixelRatio() : 1.0 : 1.0;
}

auto Util::listCorners(qint32 idx, qint32 count, qint32 radius) noexcept -> CornersGroup {
    return corners(idx == 0 ? radius : 0, idx + 1 == count ? radius : 0);
}

auto Util::tableWithHeaderCorners(qint32 row, qint32 column, qint32 rows, qint32 columns,
                                  qint32 radius) noexcept -> CornersGroup {
    CornersGroup out;
    bool         row_end      = row + 1 == rows;
    bool         column_start = column == 0;
    bool         column_end   = column + 1 == columns;
    out.setBottomLeft(row_end && column_start ? radius : 0);
    out.setBottomRight(row_end && column_end ? radius : 0);
    return out;
}

auto Util::tableCorners(qint32 row, qint32 column, qint32 rows, qint32 columns,
                        qint32 radius) noexcept -> CornersGroup {
    CornersGroup out;
    bool         row_start    = row == 0;
    bool         row_end      = row + 1 == rows;
    bool         column_start = column == 0;
    bool         column_end   = column + 1 == columns;
    out.setTopLeft(row_start && column_start ? radius : 0);
    out.setTopRight(row_start && column_end ? radius : 0);
    out.setBottomLeft(row_end && column_start ? radius : 0);
    out.setBottomRight(row_end && column_end ? radius : 0);
    return out;
}

auto Util::cornerArray(QVariant in) noexcept -> CornersGroup {
    CornersGroup out;
    if (in.canConvert<qreal>()) {
        out = CornersGroup(in.value<qreal>());
    } else if (auto list = in.toList(); ! list.empty()) {
        switch (list.size()) {
        case 1: {
            out = CornersGroup(list[0].value<qreal>());
            break;
        }
        case 2: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[0].value<qreal>());
            out.setBottomLeft(list[1].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        case 3: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        default:
        case 4: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[3].value<qreal>());
        }
        }
    }
    return out;
}

auto Util::corners(qreal in) noexcept -> CornersGroup { return CornersGroup(in); }

auto Util::corners(qreal a, qreal b) noexcept -> CornersGroup {
    CornersGroup out;
    out.setTopLeft(a);
    out.setTopRight(a);
    out.setBottomLeft(b);
    out.setBottomRight(b);
    return out;
}

auto Util::corners(qreal tl, qreal tr, qreal bl, qreal br) noexcept -> CornersGroup {
    return CornersGroup(br, tr, bl, tl);
}

void Util::track(QVariant, Track t) {
    switch (t) {
    case TrackCreate:
        m_tracked++;
        std::printf("track create %lu", m_tracked);
        break;
    case TrackDelete:
        m_tracked--;
        std::printf("track delete %lu", m_tracked);
        break;
    }
}

QString Util::type_str(const QJSValue& obj) {
    if (obj.isQObject()) {
        return QString::fromLatin1(obj.toQObject()->metaObject()->className());
    }
    if (auto v = obj.toVariant(); v.isValid()) {
        return QString::fromLatin1(v.metaType().name());
    }
    if (auto objname = obj.property(QStringLiteral("objectName")).toString(); ! objname.isEmpty()) {
        return objname;
    }
    return obj.toString();
}

void Util::print_parents(const QJSValue& obj) {
    auto cur           = obj;
    auto format_parent = ycore::y_combinator {
        [this](auto format_parent, const QJSValue& cur, i32 level) -> std::string {
            if (! cur.isNull()) {
                return std::format("    {}\n{}",
                                   type_str(cur).toStdString(),
                                   format_parent(cur.property(QStringLiteral("parent")), level + 1));
            }
            return {};
        }
    };
    qCDebug(qml_material_logcat()) << std::format(
        "{}\n{}", type_str(obj).toStdString(), format_parent(obj.property(QStringLiteral("parent")), 1));
}

auto Util::lightness(QColor color) noexcept-> qreal { return color.lightnessF(); }

auto Util::tokenElevation() noexcept -> token::Elevation { return token::Elevation(); }
auto Util::tokenShape() noexcept -> token::Shape { return token::Shape(); }
auto Util::tokenState() noexcept -> token::State { return token::State(); }

QObject* Util::createItem(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent) {
    return qcm::createItem(qmlEngine(this), url_or_comp, props, parent);
}
QObject* Util::showPopup(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent,
                         bool open_and_destry) {
    auto popup = createItem(url_or_comp, props, parent);
    if (open_and_destry) {
        QObject::connect(popup, SIGNAL(closed()), this, SLOT(onPopupClosed()));
        QMetaObject::invokeMethod(popup, "open");
    }
    return popup;
}

void Util::onPopupClosed() {
    auto s = sender();
    if (auto engine = qmlEngine(s)) {
        auto js = engine->toManagedValue(s);
        if (auto p = js.property(QStringLiteral("destroy")); p.isCallable()) {
            p.call({ 1000 });
        }
    }
}

auto Util::paramsString(const QVariantMap& in_props) -> QString {
    auto props = in_props;
    for (const auto& el : props.keys()) {
        auto val = props.value(el);
        if (auto pp = get_if<QObject*>(&val)) {
            props[el] = QString::number((std::intptr_t)(*pp));
        } else if (auto pp = get_if<QVariantMap>(&val)) {
            props[el] = paramsString(*pp);
        } else {
            props[el] = val.toString();
        }
    }
    QJsonDocument doc;
    doc.setObject(QJsonObject::fromVariantMap(props));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void Util::setCursor(QQuickItem* item, Qt::CursorShape shape) {
    if (item) {
        item->setCursor(shape);
    }
}

double Util::clamp(double t, double low, double heigh) { return std::clamp(t, low, heigh); }
double Util::teleportCurve(double t, double left, double right) {
    if (t < left) {
        double x = t / left;
        return 1.0 - easeInOut(x);
    } else if (t < right) {
        return 0.0;
    } else if (t <= 1.0) {
        double x = (t - right) / (1.0 - right);
        return easeInOut(x);
    } else {
        return 1.0;
    }
}

void Util::forceSetImplicitHeight(QQuickItem* item, qreal height) {
    if (item) {
        item->setImplicitHeight(height);
    }
}
void Util::forceSetImplicitWidth(QQuickItem* item, qreal width) {
    if (item) {
        item->setImplicitWidth(width);
    }
}

void Util::cellHoveredOn(QQuickItem* item, bool hovered, qint32 row, qint32 column) {
    if (item == nullptr) return;
    if (! hovered) return;

    auto meta       = item->metaObject();
    auto signal_sig = QMetaObject::normalizedSignature("cellHovered(qint32, qint32)");
    auto idx        = meta->indexOfSignal(signal_sig.constData());
    if (idx != -1) {
        auto signal = meta->method(idx);
        signal.invoke(item, row, column);
    }
}

QObject* Util::getParent(QObject* obj) { return obj ? obj->parent() : nullptr; }
bool     Util::disconnectAll(QObject* obj, const QString& name) {
    if (! obj) {
        qCWarning(qml_material_logcat()) << "disconnectAll: obj is null";
        return false;
    }
    auto signal_idx = obj->metaObject()->indexOfSignal(
        QMetaObject::normalizedSignature(name.toUtf8().constData()).constData());
    if (signal_idx == -1) {
        qCWarning(qml_material_logcat()) << "disconnectAll: signal not found" << name;
        return false;
    }
    auto signal = obj->metaObject()->method(signal_idx);
    return QObject::disconnect(obj, signal, nullptr, QMetaMethod {});
}

} // namespace qml_material

auto qml_material::tryCreateComponent(const QVariant& val, QQmlComponent::CompilationMode useAsync,
                                      const std::function<QQmlComponent*()>& createComponent)
    -> QQmlComponent* {
    QQmlComponent* component = nullptr;
    if (component = val.value<QQmlComponent*>(); component) {
        return component;
    } else {
        if (val.typeId() == QMetaType::QString) {
            const auto str = val.toString();
            if (str.startsWith(u"qrc:/") || str.startsWith(u"file:/")) {
                component = createComponent();
                QUrl url(str);
                component->loadUrl(url, useAsync);
            } else if (auto splits = str.split(QLatin1Char('/')); splits.size() == 2) {
                component = createComponent();
                component->loadFromModule(splits[0], splits[1], useAsync);
            }
        } else if (val.typeId() == QMetaType::QUrl) {
            component      = createComponent();
            const auto url = val.toUrl();
            component->loadUrl(url, useAsync);
        }
        if (! component) {
            qCWarning(qml_material_logcat()) << "can't create component from" << val;
        }
        return component;
    }
}

namespace qcm
{

auto qml_dyn_count() -> std::atomic<i32>& {
    static std::atomic<i32> n { 0 };
    return n;
}

auto createItem(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                QObject* parent) -> QObject* {
    std::unique_ptr<QQmlComponent, void (*)(QQmlComponent*)> comp { nullptr, nullptr };
    if (auto p = qobject_cast<QQmlComponent*>(url_or_comp.toQObject())) {
        comp = decltype(comp)(p, [](QQmlComponent*) {
        });
    } else if (auto p = url_or_comp.toVariant(); ! p.isNull()) {
        auto raw = qml_material::tryCreateComponent(p, QQmlComponent::PreferSynchronous, [engine] {
            return new QQmlComponent(engine, nullptr);
        });
        if (raw) {
            comp = decltype(comp)(raw, [](QQmlComponent* q) {
                delete q;
            });
        }
    } else {
        qCWarning(qml_material_logcat()) << "can't create component from" << url_or_comp.toString();
        return nullptr;
    }

    switch (comp->status()) {
    case QQmlComponent::Status::Ready: {
        QObject* obj { nullptr };
        QMetaObject::invokeMethod(comp.get(),
                                  "createObject",
                                  Q_RETURN_ARG(QObject*, obj),
                                  Q_ARG(QObject*, parent),
                                  Q_ARG(const QVariantMap&, props));
        if (obj != nullptr) {
            qml_dyn_count()++;
            // auto name = obj->metaObject()->className();
            QObject::connect(obj, &QObject::destroyed, [](QObject*) {
                qml_dyn_count()--;
            });
        } else {
            qCCritical(qml_material_logcat) << comp->errorString();
        }
        return obj;
        break;
    }
    case QQmlComponent::Status::Error: {
        qCCritical(qml_material_logcat) << comp->errorString();
        break;
    }
    default: break;
    }
    return nullptr;
}

} // namespace qcm

#include "qml_material/util/moc_qml_util.cpp"