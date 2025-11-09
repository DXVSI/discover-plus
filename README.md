# Discover Plus

Улучшенная версия KDE Discover с расширенной поддержкой RPM-пакетов для Fedora Linux.

![Discover window](https://cdn.kde.org/screenshots/plasma-discover/plasma-discover.png)

## Основные изменения

* **Полная поддержка RPM Fusion** - пакеты из репозиториев RPM Fusion теперь отображаются в Discover
* **Улучшенный поиск пакетов** - добавлен fallback поиск через PackageKit для пакетов без AppStream метаданных
* **Отображение источников** - показывает название репозитория для каждого пакета (Fedora Linux, RPM Fusion, COPR, Google Chrome и т.д.)
* **Правильные иконки** - используются иконки из пакетов вместо дефолтных
* **Исправлен UI** - кнопка установки теперь всегда справа независимо от наличия рейтинга

## Сборка и установка

### Зависимости

```bash
sudo dnf install -y cmake extra-cmake-modules gcc-c++ \
    kf6-kconfig-devel kf6-kcoreaddons-devel kf6-kcrash-devel \
    kf6-kdbusaddons-devel kf6-ki18n-devel kf6-karchive-devel \
    kf6-kxmlgui-devel kf6-kio-devel kf6-kcmutils-devel \
    kf6-kidletime-devel kf6-purpose-devel kf6-kiconthemes-devel \
    kf6-kstatusnotifieritem-devel kf6-kauth-devel kf6-knotifications-devel \
    kf6-kirigami-devel kf6-kirigami-addons-devel \
    PackageKit-Qt6-devel appstream-qt-devel qcoro-qt6-devel \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwebview-devel
```

### Сборка

```bash
mkdir build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_TESTING=OFF \
    -DBUILD_FlatpakBackend=ON \
    -DBUILD_PackageKitBackend=ON \
    -DBUILD_FwupdBackend=ON \
    -DBUILD_SnapBackend=ON \
    -DBUILD_AlpineApkBackend=ON \
    -DBUILD_DummyBackend=OFF \
    -DBUILD_RpmOstreeBackend=OFF \
    -DBUILD_SteamOSBackend=OFF \
    -DBUILD_WITH_QT6=ON
make -j$(nproc)
```

### Установка

```bash
sudo make install
```
## Особенности работы

### Маппинг репозиториев

- `fedora`, `updates`, `updates-testing` → "Fedora Linux"
- `rpmfusion-*` → "RPM Fusion"
- `@copr:*`, `copr:*` → "COPR"
- Другие репозитории отображаются как есть (например, "google-chrome")

### AppStream Pool флаги

Используются флаги:
- `FlagLoadOsCatalog` - загрузка системного каталога приложений
- `FlagLoadOsDesktopFiles` - загрузка .desktop файлов
- `FlagLoadOsMetainfo` - загрузка метаинформации

## Поддержка

Для вопросов и багов используйте Issues на GitHub.

## Оригинальный Discover

Этот проект основан на KDE Discover. Оригинальный проект: https://invent.kde.org/plasma/discover

## Building (Original)

The easiest way to make changes and test Discover during development is to [build it with kde-builder](https://community.kde.org/Get_Involved/development).

## Vendor Customization

Want to change the apps featured in the Editor's Choice section? Add a configuration file named `/usr/share/discover/featuredurlrc` that points to a JSON file patterned off the default one present at https://autoconfig.kde.org/discover/featured-5.9.json:
```toml
[Software]
FeaturedListingURL="https://your-url-here/file.json"
```

## Contributing

Like other projects in the KDE ecosystem, contributions are welcome from all. This repository is managed in [KDE Invent](https://invent.kde.org/plasma/discover), our GitLab instance.

* Want to contribute code? See the [GitLab wiki page](https://community.kde.org/Infrastructure/GitLab) for a tutorial on how to send a merge request.
* Reporting a bug? Please submit it on the [KDE Bugtracking System](https://bugs.kde.org/enter_bug.cgi?format=guided&product=Discover). Please do not use the Issues
  tab to report bugs.
* Is there a part of Discover that's not translated? See the [Getting Involved in Translation wiki page](https://community.kde.org/Get_Involved/translation) to see how
  you can help translate!

If you get stuck or need help with anything at all, head over to the [KDE New Contributors room](https://go.kde.org/matrix/#/#kde-welcome:kde.org) on Matrix. For questions about Discover, please ask in the [Plasma Discover room](https://go.kde.org/matrix/#/#plasma-discover:kde.org). See [Matrix](https://community.kde.org/Matrix) for more details.

