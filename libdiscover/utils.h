/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QElapsedTimer>
#include <QJsonValue>
#include <QScopeGuard>
#include <QString>
#include <functional>

class OneTimeAction : public QObject
{
public:
    // the int argument is only so the compile knows to tell the constructors apart
    OneTimeAction(int /*unnecessary*/, const std::function<bool()> &func, QObject *parent)
        : QObject(parent)
        , m_function(func)
    {
    }

    OneTimeAction(const std::function<void()> &func, QObject *parent)
        : QObject(parent)
        , m_function([func] {
            func();
            return true;
        })
    {
    }

    void trigger()
    {
        if (m_done) {
            return;
        }
        m_done = m_function();
        deleteLater();
    }

private:
    std::function<bool()> m_function;
    bool m_done = false;
};

template<typename T, typename Q, typename _UnaryOperation>
static T kTransform(const Q &input, _UnaryOperation op)
{
    T ret;
    ret.reserve(input.size());
    for (const auto &v : input) {
        ret += op(v);
    }
    return ret;
}

template<typename T, typename Q>
static T kTransform(const Q &input)
{
    T ret;
    ret.reserve(input.size());
    for (const auto &v : input) {
        ret += v;
    }
    return ret;
}

template<typename T, typename Q, typename _UnaryOperation>
static T kAppend(const Q &input, _UnaryOperation op)
{
    T ret;
    ret.reserve(input.size());
    for (const auto &v : input) {
        ret.append(op(v));
    }
    return ret;
}

template<typename T, typename Q, typename _UnaryOperation>
static T kFilter(const Q &input, _UnaryOperation op)
{
    T ret;
    for (const auto &v : input) {
        if (op(v)) {
            ret += v;
        }
    }
    return ret;
}

template<typename T, typename Q, typename _UnaryOperation>
static void kFilterInPlace(Q &input, _UnaryOperation op)
{
    for (auto it = input.begin(); it != input.end();) {
        if (op(*it)) {
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

template<typename T, typename Q, typename _UnaryOperation>
static T kFilterTransform(Q &input, _UnaryOperation op)
{
    T ret;
    for (const auto &v : input) {
        const std::optional<typename T::value_type> result = op(v);
        if (result.has_value()) {
            ret += result.value();
        }
    }
    return ret;
}

template<typename Q, typename W>
static int kIndexOf(const Q &list, W func)
{
    int i = 0;
    for (auto it = list.constBegin(), itEnd = list.constEnd(); it != itEnd; ++it) {
        if (func(*it)) {
            return i;
        }
        ++i;
    }
    return -1;
}

template<typename T>
static QVector<T> kSetToVector(const QSet<T> &set)
{
    QVector<T> ret;
    ret.reserve(set.size());
    for (auto &x : set) {
        ret.append(x);
    }
    return ret;
}

template<typename T>
static QList<T> kSetToList(const QSet<T> &set)
{
    QList<T> ret;
    ret.reserve(set.size());
    for (auto &x : set) {
        ret.append(x);
    }
    return ret;
}

template<typename T>
static QSet<T> kToSet(const QList<T> &set)
{
    return QSet<T>(set.begin(), set.end());
}

template<typename T>
static void kRemoveDuplicates(QList<T> &input)
{
    QSet<T> ret;
    for (auto it = input.begin(); it != input.end();) {
        if (!ret.contains(*it)) {
            ret << *it;
            ++it;
        } else {
            it = input.erase(it);
        }
    }
}

class ElapsedDebug : private QElapsedTimer
{
public:
    explicit ElapsedDebug(const QString &name = QStringLiteral("<unnamed>"))
        : m_name(name)
    {
        start();
    }
    ~ElapsedDebug()
    {
        qDebug("elapsed %s: %lld!", m_name.toUtf8().constData(), elapsed());
    }
    void step(const QString &step)
    {
        qDebug("step %s(%s): %lld!", m_name.toUtf8().constData(), qPrintable(step), elapsed());
    }

    QString m_name;
};

inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

template<typename T>
class KeyValueRange
{
public:
    KeyValueRange(T &data)
        : m_data{data}
    {
    }

    auto begin()
    {
        return m_data.keyValueBegin();
    }

    auto end()
    {
        return m_data.keyValueEnd();
    }

private:
    T &m_data;
};
