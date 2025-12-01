#pragma once
#include <QObject>
#include <QColor>
#include <QtDBus/QDBusVariant>

namespace qml_material
{
class Xdp : public QObject {
    Q_OBJECT
public:
    Xdp(QObject* parent = nullptr);
    ~Xdp();

    static Xdp* insance();

    QColor          accentColor() const;
    Qt::ColorScheme colorScheme() const;
public Q_SLOTS:
    void xdpSettingChangeSlot(QString, QString, QDBusVariant);

Q_SIGNALS:
    void colorSchemeChanged();
    void accentColorChanged();

private:
    std::optional<Qt::ColorScheme> m_color_scheme;
    std::optional<QColor>          m_accent_color;
};
} // namespace qml_material