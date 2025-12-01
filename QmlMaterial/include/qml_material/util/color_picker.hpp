#pragma once

#include <QtQml/QQmlEngine>
#include <QtGui/QImage>
#include <QtCore/QRunnable>

namespace qml_material
{

class ColorPickerRunnable : public QObject, public QRunnable {
    Q_OBJECT
public:
    void run() override;

    Q_SIGNAL void finished(QColor);

    QImage image;
};

class ColorPicker : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged FINAL)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged FINAL)

public:
    ColorPicker(QObject* = nullptr);
    ~ColorPicker();

    auto image() const -> QImage;
    void setImage(QImage);

    auto color() const noexcept -> QColor;
    void setColor(QColor);

    Q_SIGNAL void pick();
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void colorChanged();

private:
    QImage m_image;
    QColor m_color;
};

} // namespace qml_material