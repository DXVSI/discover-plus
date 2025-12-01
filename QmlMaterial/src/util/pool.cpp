#include "qml_material/util/pool.hpp"
#include "qml_material/util/loggingcategory.hpp"
#include "qml_material/util/qml_util.hpp"

namespace
{
auto variant_map_to_string(const QVariantMap& m) -> QString {
    QString out;
    for (const auto& el : m.asKeyValueRange()) {
        out.append(QString(QStringLiteral("%1%2")).arg(el.first).arg(el.second.toString()));
    }
    return out;
}
} // namespace

namespace qml_material
{
PoolIncubator::PoolIncubator(Pool* p, qint64 id, IncubationMode mode)
    : QQmlIncubator(mode), m_pool(p), m_id(id) {}
void PoolIncubator::statusChanged(Status s) {
    QQmlIncubator::statusChanged(s);
    m_pool->incubatorStateChanged(m_id, s);
}
void PoolIncubator::setInitialState(QObject* o) {
    QQmlIncubator::setInitialState(o);
    m_pool->setInitialState(o);
}

Pool::Pool(QObject* parent): QObject(parent), m_serial(0), m_async(true) {
    connect(this, &Pool::queueAdded, this, &Pool::onQueueAdded, Qt::QueuedConnection);
}
Pool::~Pool() {
    for (const auto& el : m_objs) {
        if (el.second) {
            delete el.second.get();
        }
    }
    for (auto el : m_nokey_objs) {
        delete el;
    }
    for (const auto& el : m_tasks) {
        if (el.second.object) {
            delete el.second.object;
        }
    }
}
void Pool::incubatorStateChanged(qint64 id, QQmlIncubator::Status status) {
    if (status == QQmlIncubator::Loading || status == QQmlIncubator::Null) return;

    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& t         = it->second;
    auto  incubator = it->second.incubator;

    if (status == QQmlIncubator::Ready) {
        auto object = incubator->object();
        incubator->clear();
        if (t.hasKey) {
            m_objs.insert({ t.key, object });
        } else {
            connect(object, &QObject::destroyed, this, &Pool::onUncacheObjectDeleted);
            m_nokey_objs.insert(object);
        }
        t.object = object;
        queueAdded(id);
    } else if (status == QQmlIncubator::Error) {
        if (! incubator->errors().isEmpty()) {
            qCWarning(qml_material_logcat()) << incubator->errors();
        }
        delete incubator->object();
        clearTask(it);
    }
}
void Pool::setInitialState(QObject*) {}

void Pool::clearTask(task_iterator it) {
    auto& t = it->second;
    if (t.incubator) {
        t.incubator->clear();
        delete t.incubator;
        t.incubator = nullptr;
    }
    if (t.component) {
        if (t.component->parent() == this) {
            t.component->deleteLater();
            t.component = nullptr;
        }
    }
    m_tasks.erase(it);
}

auto     Pool::genSerial() const -> qint64 { return m_serial++; }
bool     Pool::contains(const QString& key) const { return m_objs.contains(key); }
QObject* Pool::get(const QString& key) const {
    if (auto it = m_objs.find(key); it != m_objs.end()) {
        return it->second;
    }
    return nullptr;
}
void Pool::add(std::optional<QStringView> key, QQmlComponent* component, const QVariantMap& props) {
    Task task {};
    task.component = component;

    auto id = genSerial();
    if (key) {
        if (tryCache(*key)) return;
        if (! component) return;

        task.key    = QString(*key);
        task.hasKey = true;
    } else {
        if (! component) return;
    }

    task.incubator = new PoolIncubator(
        this, id, async() ? QQmlIncubator::Asynchronous : QQmlIncubator::Synchronous);
    task.incubator->setInitialProperties(props);
    m_tasks.insert({ id, task });

    if (! component->isLoading()) {
        onComponentLoaded(id);
    } else {
        QObject::connect(component, &QQmlComponent::statusChanged, this, [this, id]() {
            onComponentLoaded(id);
        });
        QObject::connect(component, &QQmlComponent::progressChanged, this, [this, id](qreal p) {
            onComponentProgress(id, p);
        });
    }
}

auto Pool::tryCreateComponent(const QVariant& val) -> QQmlComponent* {
    auto useAsync = async() ? QQmlComponent::Asynchronous : QQmlComponent::PreferSynchronous;
    return qml_material::tryCreateComponent(val, useAsync, [this] {
        return createComponent();
    });
}
void Pool::addWithKey(const QString& key, const QVariant& val, const QVariantMap& props) {
    if (tryCache(key)) return;
    if (auto comp = tryCreateComponent(val)) {
        add(key, comp, props);
    }
}

auto Pool::createComponent() -> QQmlComponent* {
    auto engine = qmlEngine(this);
    if (engine == nullptr) return nullptr;
    return new QQmlComponent(engine, this);
}

void Pool::add(const QVariant& val, const QVariantMap& props, bool autoKey) {
    if (autoKey) {
        auto key = QString(QStringLiteral("%1%2")).arg(val.toString()).arg(variant_map_to_string(props));
        addWithKey(key, val, props);
    } else {
        if (auto comp = tryCreateComponent(val)) {
            add(std::nullopt, comp, props);
        }
    }
}

void Pool::onComponentProgress(qint64, qreal) {}
void Pool::onComponentLoaded(qint64 id) {
    if (auto it = m_tasks.find(id); it != m_tasks.end()) {
        auto& t = it->second;
        if (t.component->isError()) {
            qCCritical(qml_material_logcat()) << t.component->errorString();
        } else {
            t.component->create(*t.incubator);
        }
    }
}
void Pool::onQueueAdded(qint64 id) {
    // try pop first task
    if (m_tasks.empty()) return;
    auto it = m_tasks.begin();
    if (it->first != id) return;

    const auto& t = it->second;
    if (t.object) {
        auto key = t.hasKey ? QVariant(t.key) : QVariant();
        qCDebug(qml_material_logcat()) << "pool: object added" << t.object << t.key;
        objectAdded(t.object, key);
        clearTask(it);

        // try next
        if (! m_tasks.empty()) {
            auto it = m_tasks.begin();
            queueAdded(it->first);
        }
    }
}
void Pool::onUncacheObjectDeleted(QObject* o) { m_nokey_objs.erase(o); }

auto Pool::async() const -> bool { return m_async; }
void Pool::setAsync(bool v) {
    if (m_async != v) {
        m_async = v;
        asyncChanged(m_async);
    }
}

bool Pool::removeObject(QObject* o) {
    if (m_nokey_objs.erase(o) > 0) {
        qCDebug(qml_material_logcat()) << "pool: object removed" << o;
        o->deleteLater();
        return true;
    }
    for (auto& el : m_objs) {
        if (el.second == o) {
            qCDebug(qml_material_logcat()) << "pool: object removed" << o << el.first;
            o->deleteLater();
            m_objs.erase(el.first);
            return true;
        }
    }
    return false;
}
void Pool::clear() {
    for (auto o : m_nokey_objs) {
        o->deleteLater();
    }
    m_nokey_objs.clear();
    for (auto o : m_objs) {
        o.second->deleteLater();
    }
    m_objs.clear();
}

auto Pool::tryCache(QStringView key) -> bool {
    if (auto it = m_objs.find(key); it != m_objs.end()) {
        auto id = genSerial();
        m_tasks.insert({ id,
                         Task {
                             .key    = key.toString(),
                             .hasKey = true,
                             .object = it->second,
                         } });
        queueAdded(id);
        return true;
    }
    return false;
}

} // namespace qml_material

#include "qml_material/util/moc_pool.cpp"