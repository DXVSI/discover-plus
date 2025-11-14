# COPR Integration для Discover

## Цель
Добавить категорию "COPR" в Discover, которая показывает все доступные пакеты из COPR репозиториев для текущей версии Fedora пользователя.

## Требования

### Функциональные требования
1. **Автоматическое определение версии Fedora**
   - Определять версию Fedora пользователя (например: `43`, `42`, `rawhide`)
   - Использовать для фильтрации доступных пакетов в COPR

2. **Категория "COPR" в боковом меню**
   - Показывать список всех доступных COPR пакетов для версии пользователя
   - Загружать данные только при открытии категории (ленивая загрузка)

3. **Отображение информации о пакетах**
   - Название пакета
   - Описание
   - Версия
   - Информация о COPR репозитории (owner/project)
   - Статус: доступен для fc43, fc42, и т.д.
   - Предупреждение если пакет недоступен для версии пользователя

4. **Установка пакетов**
   - При клике "Установить" автоматически подключать COPR репозиторий
   - Использовать команду: `dnf copr enable owner/project`
   - После подключения устанавливать пакет через PackageKit

## Технические детали

### API
Использовать COPR API v3: https://copr.fedorainfracloud.org/api_3/docs

#### Основные endpoints:

1. **Получить список проектов**
   ```
   GET /api_3/project/search?query={search_term}
   ```

2. **Получить информацию о проекте**
   ```
   GET /api_3/project?ownername={owner}&projectname={project}
   ```
   Возвращает список доступных chroots (fedora-43-x86_64, fedora-42-x86_64, и т.д.)

3. **Получить список пакетов в проекте**
   ```
   GET /api_3/package?ownername={owner}&projectname={project}
   ```

4. **Поиск пакетов**
   ```
   GET /api_3/package/search?query={package_name}
   ```

### Определение версии Fedora

```cpp
QString getFedoraVersion() {
    QFile file("/etc/os-release");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("VERSION_ID=")) {
                return line.split("=")[1].remove("\"");
            }
        }
    }
    return "43"; // fallback
}

QString getChroot() {
    QString version = getFedoraVersion();
    QString arch = QSysInfo::currentCpuArchitecture(); // "x86_64", "aarch64"

    if (version == "rawhide") {
        return QString("fedora-rawhide-%1").arg(arch);
    }
    return QString("fedora-%1-%2").arg(version, arch);
}
```

### Структура данных

```cpp
struct CoprPackage {
    QString name;
    QString description;
    QString owner;
    QString projectName;
    QString version;
    QStringList availableChroots; // ["fedora-43-x86_64", "fedora-42-x86_64"]
    bool isAvailableForCurrentFedora;
};

struct CoprProject {
    QString owner;
    QString name;
    QString description;
    QStringList chroots;
};
```

## Архитектура

### Вариант 1: Расширение PackageKitBackend (рекомендуется)
- Добавить `CoprSource` класс в PackageKitBackend
- При открытии категории "COPR" запрашивать данные через API
- Создавать виртуальные `PackageKitResource` для COPR пакетов
- При установке сначала подключать репозиторий через `dnf copr enable`

### Вариант 2: Отдельный CoprBackend
- Создать новый backend `CoprBackend` наследующий `AbstractResourcesBackend`
- Полностью независимая реализация
- Больше контроля, но дублирование кода

## План реализации

### Этап 1: Базовая инфраструктура (простой вариант)
1. ✅ Добавить категорию "COPR" в боковое меню (hardcoded)
2. Создать класс `CoprClient` для работы с API
3. При открытии категории показывать пакеты из **уже подключенных** COPR репозиториев
4. Фильтрация через PackageKit: `origin` содержит "copr"

### Этап 2: COPR API Integration
1. Реализовать `CoprClient::searchPackages(QString query)`
2. Реализовать `CoprClient::getProjectInfo(QString owner, QString project)`
3. Парсинг JSON ответов
4. Кэширование результатов

### Этап 3: UI для COPR пакетов
1. Показывать доступность для текущей версии Fedora
2. Предупреждения о неофициальных репозиториях
3. Информация о количестве загрузок, рейтинг проекта

### Этап 4: Автоматическое подключение репозиториев
1. При клике "Установить" показывать диалог:
   ```
   Этот пакет из COPR репозитория:
   Owner: @user/project
   Репозиторий будет подключен автоматически.
   COPR репозитории не поддерживаются официально Fedora.
   Продолжить?
   ```
2. Выполнять `dnf copr enable owner/project`
3. Устанавливать пакет через PackageKit

### Этап 5: Оптимизация
1. Ленивая загрузка (пагинация)
2. Кэширование списка проектов
3. Фоновое обновление данных

## Безопасность

1. **Предупреждения пользователю**
   - COPR репозитории не проверяются Fedora
   - Могут содержать нестабильные или небезопасные пакеты
   - Использовать на свой риск

2. **Проверка версии**
   - Показывать предупреждение если пакет недоступен для версии пользователя
   - Блокировать установку если chroot недоступен

3. **Подключение репозиториев**
   - Спрашивать подтверждение перед `dnf copr enable`
   - Показывать информацию о владельце репозитория

## Примеры использования

### Пример 1: Поиск в COPR
```
Пользователь открывает категорию "COPR"
→ Отправляется запрос к API: GET /api_3/project/search
→ Показывается список популярных проектов
→ Пользователь ищет "zen-browser"
→ Находится проект @sneexy/zen-browser
→ Показывается: "Доступно для: fc43, fc42, fc41"
→ Кнопка "Установить" активна (fc43 доступен)
```

### Пример 2: Установка из COPR
```
Пользователь кликает "Установить" на zen-browser
→ Диалог: "Подключить COPR репозиторий @sneexy/zen-browser?"
→ Пользователь подтверждает
→ Выполняется: dnf copr enable sneexy/zen-browser
→ Репозиторий подключен
→ Устанавливается пакет через PackageKit
```

### Пример 3: Пакет недоступен для fc43
```
Пользователь находит старый проект
→ Показывается: "Доступно для: fc40, fc41" (красным)
→ Предупреждение: "Этот пакет не собран для Fedora 43"
→ Кнопка "Установить" неактивна
```

## Файлы для изменения

### C++ Backend
```
/libdiscover/backends/PackageKitBackend/
  ├── CoprClient.h          (новый)
  ├── CoprClient.cpp        (новый)
  ├── CoprResource.h        (новый)
  ├── CoprResource.cpp      (новый)
  └── PackageKitBackend.cpp (изменить - добавить COPR поддержку)
```

### QML UI
```
/discover/qml/
  ├── DiscoverDrawer.qml    (добавить категорию COPR)
  ├── CoprPage.qml          (новая страница для COPR)
  └── CoprDelegate.qml      (делегат для отображения COPR пакета)
```

## Зависимости

- Qt Network для HTTP запросов к API
- Qt JSON для парсинга ответов
- `dnf` и `copr` plugin установлены в системе

## Тестирование

1. Проверка определения версии Fedora
2. Тестирование API запросов
3. Проверка подключения репозиториев
4. Тестирование установки пакетов
5. Проверка предупреждений о безопасности

## Будущие улучшения

1. Статистика популярности COPR пакетов
2. Рейтинги и отзывы
3. История обновлений проектов
4. Поддержка других дистрибутивов (CentOS Stream, RHEL)
5. Интеграция с Fedora Account System (FAS) для авторизации

## build 
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
  make -j32 && sudo make install