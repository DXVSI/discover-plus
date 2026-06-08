/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <functional>

template<typename T>
class EmitWhenChanged : public QObject
{
public:
    EmitWhenChanged(T initial, const std::function<T()> &get, const std::function<void(T)> &emitChanged)
        : m_get(get)
        , m_emitChanged(emitChanged)
        , m_value(initial)
    {
    }

    void reevaluate()
    {
        const T newValue = m_get();
        if (newValue != m_value) {
            m_value = newValue;
            m_emitChanged(m_value);
        }
    }

    std::function<T()> const m_get;
    std::function<void(T)> const m_emitChanged;
    T m_value;
};
