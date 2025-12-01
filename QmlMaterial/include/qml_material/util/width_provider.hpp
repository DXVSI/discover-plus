#pragma once
#include <QtQml/QQmlEngine>

namespace qml_material
{

class WidthProvider : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(qint32 column READ column NOTIFY columnChanged FINAL)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum NOTIFY minimumChanged FINAL)
    Q_PROPERTY(double leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(
        double rightMargin READ rightMargin WRITE setrightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(double spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(double total READ total WRITE setTotal NOTIFY totalChanged FINAL)
public:
    explicit WidthProvider(QObject* parent = nullptr);

    Q_INVOKABLE qint32 calculateX(qint32 column) noexcept;

    auto          width() const noexcept -> double;
    Q_SIGNAL void widthChanged();

    auto          column() const noexcept -> qint32;
    Q_SIGNAL void columnChanged();

    auto          leftMargin() const noexcept -> double;
    void          setLeftMargin(double);
    Q_SIGNAL void leftMarginChanged();

    auto          rightMargin() const noexcept -> double;
    void          setrightMargin(double);
    Q_SIGNAL void rightMarginChanged();

    auto          minimum() const noexcept -> double;
    void          setMinimum(double);
    Q_SIGNAL void minimumChanged();

    auto          spacing() const noexcept -> double;
    void          setSpacing(double);
    Q_SIGNAL void spacingChanged();

    auto          total() const noexcept -> double;
    void          setTotal(double);
    Q_SIGNAL void totalChanged();

    Q_SLOT void refresh();

private:
    qint32 m_column;
    double m_width;
    double m_total;
    double m_minimum;
    double m_spacing;
    double m_left_margin;
    double m_right_margin;
};

} // namespace qml_material