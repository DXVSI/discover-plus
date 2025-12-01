#pragma once

#include <QtCore/QEasingCurve>
#include <QtQml/QQmlEngine>

#include "qml_material/core.hpp"

namespace qml_material
{

class CircularIndicatorUpdator : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double startFraction READ startFraction NOTIFY updated FINAL)
    Q_PROPERTY(double endFraction READ endFraction NOTIFY updated FINAL)
    Q_PROPERTY(double rotation READ rotation NOTIFY updated FINAL)
    Q_PROPERTY(double progress READ progress WRITE update NOTIFY updated FINAL)
    Q_PROPERTY(double completeEndProgress READ completeEndProgress WRITE updateCompleteEndProgress
                   NOTIFY updated FINAL)
    Q_PROPERTY(double duration READ duration NOTIFY indeterminateAnimationTypeChanged FINAL)
    Q_PROPERTY(double completeEndDuration READ completeEndDuration NOTIFY indeterminateAnimationTypeChanged FINAL)
    Q_PROPERTY(
        IndeterminateAnimationType indeterminateAnimationType READ indeterminateAnimationType WRITE
            setIndeterminateAnimationType NOTIFY indeterminateAnimationTypeChanged FINAL)

public:
    CircularIndicatorUpdator(QObject* parent = nullptr);

    enum class IndeterminateAnimationType
    {
        Advance = 0,
        Reteat,
    };
    Q_ENUM(IndeterminateAnimationType)

    auto startFraction() const noexcept -> double;
    auto endFraction() const noexcept -> double;
    auto rotation() const noexcept -> double;

    auto progress() const noexcept -> double;
    auto completeEndProgress() const noexcept -> double;
    auto duration() const noexcept -> double;
    auto completeEndDuration() const noexcept -> double;

    auto indeterminateAnimationType() const noexcept -> IndeterminateAnimationType;
    void setIndeterminateAnimationType(IndeterminateAnimationType t);

    void updateAdvance(double progress) noexcept;
    void updateRetreat(double progress) noexcept;

    Q_SLOT void   update(double progress);
    Q_SLOT void   updateCompleteEndProgress(double progress);
    Q_SIGNAL void updated();
    Q_SIGNAL void indeterminateAnimationTypeChanged();

private:
    IndeterminateAnimationType m_type;
    QEasingCurve               m_curve;
    double                     m_progress;

    float m_start_fraction;
    float m_end_fraction;
    float m_rotation_degree;

    float m_complete_end_progress;
};
} // namespace qml_material