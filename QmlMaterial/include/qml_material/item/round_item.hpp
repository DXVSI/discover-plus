#pragma once

#include <QQuickItem>
#include <QQmlEngine>

namespace qml_material {
    class RoundItem : public QQuickItem {
        Q_OBJECT
        QML_ELEMENT
    public:
        RoundItem(QQuickItem* parent = nullptr);
        ~RoundItem() override;


    protected:
        QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    };
}