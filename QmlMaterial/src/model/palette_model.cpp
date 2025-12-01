#include "qml_material/model/palette_model.hpp"

namespace qml_material
{

PaletteModel::PaletteModel(QObject* parent): QAbstractListModel(parent) {}

int PaletteModel::rowCount(const QModelIndex&) const {
    auto meta = QMetaEnum::fromType<Enum::PaletteType>();
    return meta.keyCount();
}
QVariant PaletteModel::data(const QModelIndex& index, int role) const {
    constexpr auto prefix = std::string_view { "Palette" }.size();
    auto           row    = index.row();
    if (row < 0 || row > rowCount()) return {};
    if (role == Qt::UserRole) {
        auto meta = QMetaEnum::fromType<Enum::PaletteType>();
        auto str  = meta.valueToKey(row);

        return QString::fromUtf8(std::string_view { str }.substr(prefix));
    } else if (role == Qt::UserRole + 1) {
        return row;
    }
    return {};
}
QHash<int, QByteArray> PaletteModel::roleNames() const {
    static QHash<int, QByteArray> map {
        { Qt::UserRole, "name" },
        { Qt::UserRole + 1, "value" },
    };
    return map;
}

} // namespace qml_material

#include <qml_material/model/moc_palette_model.cpp>