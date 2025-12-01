#include "qml_material/model/snake_model.hpp"
#include <QTimer>

namespace
{
auto snake_model_role_names() -> const QHash<int, QByteArray>& {
    static QHash<int, QByteArray> map {
        { Qt::UserRole, "sid" },          { Qt::UserRole + 1, "text" },
        { Qt::UserRole + 2, "duration" }, { Qt::UserRole + 3, "flag" },
        { Qt::UserRole + 4, "action" },
    };
    return map;
}
} // namespace

namespace qml_material
{
SnakeModel::SnakeModel(QObject* parent): QAbstractListModel(parent) {
    connect(this,
            &QAbstractListModel::rowsRemoved,
            this,
            &SnakeModel::onRemovePriv,
            Qt::QueuedConnection);
}
SnakeModel::~SnakeModel() {}

int SnakeModel::rowCount(const QModelIndex&) const { return m_items.size(); }

QVariant SnakeModel::data(const QModelIndex& index, int role) const {
    auto  idx = index.row();
    auto& rn  = snake_model_role_names();
    if (idx < rowCount() && rn.contains(role)) {
        auto& name = rn[role];
        auto  prop = SnakeModelItem::staticMetaObject.property(
            SnakeModelItem::staticMetaObject.indexOfProperty(name.constData()));
        if (prop.isValid()) {
            auto& item = m_items.at(idx);
            return prop.readOnGadget(&item);
        }
    }
    return {};
}
QHash<int, QByteArray> SnakeModel::roleNames() const { return snake_model_role_names(); }

SnakeModelItem SnakeModel::createSnake() { return { (m_snake_id++) % 1000 }; }

auto SnakeModel::findById(qint32 id) -> iterator {
    return std::find_if(m_items.begin(), m_items.end(), [id](const auto& el) {
        return id == el.id;
    });
}

void SnakeModel::removeById(qint32 id) {
    {
        auto it = findById(id);
        if (it != m_items.end()) {
            auto idx = it - m_items.begin();
            removeRow(idx);
        }
    }
    {
        auto it = std::find_if(m_saved_items.begin(), m_saved_items.end(), [id](const auto& el) {
            return id == el.id;
        });
        if (it != m_saved_items.end()) {
            m_saved_items.erase(it);
        }
    }
}

void SnakeModel::showSnake(const SnakeModelItem& item) {
    beginInsertRows({}, 0, 0);
    {
        m_items.insert(m_items.begin(), item);
        auto& cur = m_items.front();
        if (item.duration == 0) {
            cur.flag |= Enum::ToastFlag::TFSave;
        }
        if (cur.flag & Enum::ToastFlag::TFSave) {
            cur.flag |= Enum::ToastFlag::TFCloseable;
        }

        if (item.duration > 0) {
            auto timer = new QTimer(this);
            connect(timer, &QTimer::timeout, timer, [id = item.id, this, timer]() {
                removeById(id);
                timer->deleteLater();
            });
            timer->setInterval(item.duration);
            timer->start();
        }
    }

    endInsertRows();

    if (m_items.size() > 1) {
        beginRemoveRows({}, 1, 1);
        auto it     = m_items.begin() + 1;
        auto remain = removeExtra(it->id);
        if (it->flag & Enum::ToastFlag::TFSave) {
            if (it->duration != 0) it->duration = std::max(remain, 1000);
            m_saved_items.push_back(*it);
        }
        m_items.erase(it);
        endRemoveRows();
    }
}

bool SnakeModel::removeRows(int row, int count, const QModelIndex&) {
    if (count <= 0) return false;
    beginRemoveRows({}, row, row + count - 1);
    auto start = m_items.begin() + row;
    auto last  = m_items.begin() + row + count;
    for (auto it = start; it < last; it++) {
        removeExtra(it->id);
    }
    m_items.erase(start, last);
    endRemoveRows();
    return true;
}

qint32 SnakeModel::removeExtra(qint32 id) {
    if (m_extra.contains(id)) {
        auto& extra = m_extra.at(id);
        if (extra.timer) {
            auto remain = extra.timer->remainingTime();
            extra.timer->deleteLater();
            extra.timer = nullptr;
            return remain;
        }
        m_extra.erase(id);
    }
    return 0;
}

void SnakeModel::onRemovePriv(const QModelIndex&, int, int) {
    if (rowCount() == 0 && ! m_saved_items.empty()) {
        auto el = m_saved_items.back();
        m_saved_items.pop_back();
        showSnake(el);
    }
}

} // namespace qml_material

#include <qml_material/model/moc_snake_model.cpp>