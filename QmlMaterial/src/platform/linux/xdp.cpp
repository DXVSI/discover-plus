#include "platform/linux/xdp.hpp"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QGlobalStatic>
#include <QGuiApplication>
//#include <QtGui/qpa/qwindowsysteminterface.h>
//#include <QtGui/qpa/qplatformtheme.h>
#include <QStyleHints>

#include "qml_material/token/color.hpp"

Q_GLOBAL_STATIC(qml_material::Xdp, TheXdp)

namespace qml_material
{
namespace
{
inline constexpr auto kService           = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath        = "/org/freedesktop/portal/desktop";
inline constexpr auto kRequestInterface  = "org.freedesktop.portal.Request";
inline constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";

QColor to_accent_color(const QVariant& in) {
    std::array<double, 3> c { 0 };
    const auto            v = in.value<QDBusArgument>();
    v.beginStructure();
    v >> c[0] >> c[1] >> c[2];
    v.endStructure();
    return QColor::fromRgbF(c[0], c[1], c[2]);
}
Qt::ColorScheme to_color_scheme(const QVariant& in) {
    auto v = in.toUInt();
    return v == 1 ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}
void convert_from(qml_material::Enum::ThemeMode& out, const Qt::ColorScheme& in) {
    using out_type = qml_material::Enum::ThemeMode;
    using in_type  = Qt::ColorScheme;
    switch (in) {
    case in_type::Dark: out = out_type::Dark; break;
    default: out = out_type::Light;
    }
}
} // namespace

Xdp::Xdp(QObject* parent): QObject(parent) {
    auto bus = QDBusConnection::sessionBus();
    auto res = bus.connect(kService,
                           kObjectPath,
                           kSettingsInterface,
                           "SettingChanged",
                           this,
                           SLOT(xdpSettingChangeSlot(QString, QString, QDBusVariant)));
    assert(res);

    auto message =
        QDBusMessage::createMethodCall(kService, kObjectPath, kSettingsInterface, "Read");

    {
        message << "org.freedesktop.appearance"
                << "color-scheme";
        // this must not be asyncCall() because we have to set appearance now
        QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(reply.value());
            m_color_scheme                 = to_color_scheme(dbusVariant.variant());
        }
    }

    {
        message.setArguments({ "org.freedesktop.appearance", "accent-color" });
        QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(reply.value());
            m_accent_color                 = to_accent_color(dbusVariant.variant());
        }
    }
}
Xdp::~Xdp() {}

Xdp* Xdp::insance() { return TheXdp; }

void Xdp::xdpSettingChangeSlot(QString namespace_, QString key, QDBusVariant value_) {
    auto value = value_.variant();
    if (namespace_ == QLatin1String("org.freedesktop.appearance") && key == QLatin1String("color-scheme")) {
        m_color_scheme = to_color_scheme(value);
        Q_EMIT colorSchemeChanged();
    } else if (namespace_ == QLatin1String("org.freedesktop.appearance") && key == QLatin1String("accent-color")) {
        m_accent_color = to_accent_color(value);
        Q_EMIT accentColorChanged();
    }
}

QColor Xdp::accentColor() const { return m_accent_color.value_or(QColor {}); }

Qt::ColorScheme Xdp::colorScheme() const {
    return m_color_scheme ? m_color_scheme.value() : QGuiApplication::styleHints()->colorScheme();
}

auto sysColorScheme() -> Enum::ThemeMode {
    Enum::ThemeMode out;
    convert_from(out, Xdp::insance()->colorScheme());
    return out;
}
auto sysAccentColor() -> QColor { return Xdp::insance()->accentColor(); }

void sysNotifyInit(MdColorMgr& mgr) {
    QObject::connect(Xdp::insance(), &Xdp::colorSchemeChanged, &mgr, &MdColorMgr::refrehFromSystem);
    QObject::connect(Xdp::insance(), &Xdp::accentColorChanged, &mgr, &MdColorMgr::refrehFromSystem);
}

} // namespace qml_material

#include <moc_xdp.cpp>