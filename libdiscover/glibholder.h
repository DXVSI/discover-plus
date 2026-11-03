/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

template<typename T>
class GLibHolder
{
public:
    GLibHolder(T *object)
        : m_object(object)
    {
        g_object_ref(object);
    }

    ~GLibHolder()
    {
        g_object_unref(m_object);
    }

    GLibHolder(const GLibHolder &other)
        : GLibHolder(other.m_object)
    {
    }

    GLibHolder(GLibHolder &&other)
        : m_object(other.m_object)
    {
        other.m_object = nullptr;
    }

    T *get() const
    {
        return m_object;
    }

private:
    T *m_object = nullptr;
};
