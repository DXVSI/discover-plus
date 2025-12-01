#pragma once

#include <QtCore/QEasingCurve>
#include <QtGui/QColor>
#include <QtQml/QQmlEngine>

#include "qml_material/core.hpp"

namespace qml_material
{

class LinearActiveIndicatorData : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double startFraction READ startFraction NOTIFY updated FINAL)
    Q_PROPERTY(double endFraction READ endFraction NOTIFY updated FINAL)
    Q_PROPERTY(QColor color READ getColor NOTIFY updated FINAL)
    Q_PROPERTY(qint32 gapSize READ getGapSize NOTIFY updated FINAL)
public:
    LinearActiveIndicatorData(QObject* parent = nullptr);
    auto startFraction() const noexcept -> double;
    auto endFraction() const noexcept -> double;
    auto getColor() const noexcept -> QColor;
    auto getGapSize() const noexcept -> qint32;

    Q_SIGNAL void updated();

    float  start_fraction = 0;
    float  end_fraction   = 0;
    QColor color {};
    qint32    gap_size = 0;
    // The fraction [0, 1] of the amplitude on indicator.
    float amplitude_fraction = 1;
    // The fraction [0, 1] of the initial phase [0, 2 * PI] on indicator.
    float phase_fraction = 0;
};

class LinearIndicatorUpdator : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged FINAL)

    Q_PROPERTY(QList<qml_material::LinearActiveIndicatorData*> activeIndicators READ
                   activeIndicators NOTIFY indeterminateAnimationTypeChanged FINAL)

    Q_PROPERTY(double progress READ progress WRITE update NOTIFY updated FINAL)
    Q_PROPERTY(double completeEndProgress READ completeEndProgress WRITE updateCompleteEndProgress
                   NOTIFY updated FINAL)

    Q_PROPERTY(double duration READ duration NOTIFY indeterminateAnimationTypeChanged FINAL)
    Q_PROPERTY(double completeEndDuration READ completeEndDuration NOTIFY
                   indeterminateAnimationTypeChanged FINAL)

    Q_PROPERTY(
        IndeterminateAnimationType indeterminateAnimationType READ indeterminateAnimationType WRITE
            setIndeterminateAnimationType NOTIFY indeterminateAnimationTypeChanged FINAL)

public:
    LinearIndicatorUpdator(QObject* parent = nullptr);

    enum class IndeterminateAnimationType
    {
        DisJoint = 0,
        Contiguous,
    };
    Q_ENUM(IndeterminateAnimationType)

    auto activeIndicators() -> QList<LinearActiveIndicatorData*>;
    auto progress() const noexcept -> double;
    auto completeEndProgress() const noexcept -> double;
    auto duration() const noexcept -> double;
    auto completeEndDuration() const noexcept -> double;

    auto colors() const -> QList<QColor>;
    void setColors(const QList<QColor>&);

    auto indeterminateAnimationType() const noexcept -> IndeterminateAnimationType;
    void setIndeterminateAnimationType(IndeterminateAnimationType t);

    void updateContiguous(double progress) noexcept;
    void updateDisjoint(double progress) noexcept;
    void updateColors() noexcept;
    Q_SLOT void initIndicatorDatas() noexcept;

    Q_SLOT void   update(double progress);
    Q_SLOT void   updateCompleteEndProgress(double progress);
    Q_SIGNAL void updated();
    Q_SIGNAL void indeterminateAnimationTypeChanged();
    Q_SIGNAL void colorsChanged();

private:
    IndeterminateAnimationType m_type;

    QEasingCurve                m_contiguous_interpolator;
    std::array<QEasingCurve, 4> m_interpolators;
    double                      m_progress;
    double                      m_complete_end_progress;

    std::array<LinearActiveIndicatorData*, 3> m_active_indicators;
    QList<QColor>                             m_colors;
    quint32                                   m_color_idx;
    bool                                      m_color_dirty;
};
} // namespace qml_material