/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DummyResource.h"
#include <KRandom>
#include <QDesktopServices>
#include <QStringList>
#include <QTimer>
#include <Transaction/AddonList.h>

Q_GLOBAL_STATIC_WITH_ARGS(
    QVector<QString>,
    s_icons,
    ({QLatin1String("kdevelop"), QLatin1String("kalgebra"), QLatin1String("kmail"), QLatin1String("akregator"), QLatin1String("korganizer")}))

DummyResource::DummyResource(QString name, AbstractResource::Type type, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_name(std::move(name))
    , m_state(State::Broken)
    , m_iconName((*s_icons)[QRandomGenerator::global()->bounded(s_icons->size())])
    , m_addons({PackageState(QStringLiteral("a"), QStringLiteral("aaaaaa"), false),
                PackageState(QStringLiteral("b"), QStringLiteral("aaaaaa"), false),
                PackageState(QStringLiteral("c"), QStringLiteral("aaaaaa"), false)})
    , m_type(type)
{
    const int nofScreenshots = QRandomGenerator::global()->bounded(5);
    m_screenshots =
        Screenshots{
            QUrl(QStringLiteral("https://screenshots.debian.net/screenshots/000/014/863/large.png")),
            QUrl(QStringLiteral("https://c1.staticflickr.com/9/8479/8166397343_b78106f353_k.jpg")),
            QUrl(QStringLiteral("https://c2.staticflickr.com/4/3685/9954407993_dad10a6943_k.jpg")),
            QUrl(QStringLiteral("https://c1.staticflickr.com/1/653/22527103378_8ce572e1de_k.jpg")),
            QUrl(QStringLiteral(
                "https://images.unsplash.com/photo-1528744598421-b7b93e12df15?ixlib=rb-1.2.1&ixid=eyJhcHBfaWQiOjEyMDd9&auto=format&fit=crop&w=500&q=60")),
            QUrl(QStringLiteral(
                "https://images.unsplash.com/photo-1552385430-53e6f2028760?ixlib=rb-1.2.1&ixid=eyJhcHBfaWQiOjEyMDd9&auto=format&fit=crop&w=634&q=80")),
            QUrl(QStringLiteral(
                "https://images.unsplash.com/photo-1506810172640-8a9f77cb1472?ixlib=rb-1.2.1&ixid=eyJhcHBfaWQiOjEyMDd9&auto=format&fit=crop&w=1350&q=80")),

        }
            .mid(nofScreenshots);
}

QList<PackageState> DummyResource::addonsInformation()
{
    return m_addons;
}

QString DummyResource::availableVersion() const
{
    return QStringLiteral("3.0");
}

bool DummyResource::hasCategory(const QString &category) const
{
    if (category == QLatin1StringView("dummy"))
        return true;
    if (m_name.endsWith(u'3') && category == QLatin1StringView("three"))
        return true;
    else if (!m_name.endsWith(u'3') && category == QLatin1StringView("notthree"))
        return true;
    return false;
}

QString DummyResource::comment()
{
    return QLatin1StringView("A reasonably short comment ") + name();
}

quint64 DummyResource::size()
{
    return m_size;
}

QUrl DummyResource::homepage()
{
    return QUrl(QStringLiteral("https://kde.org"));
}

QUrl DummyResource::helpURL()
{
    return QUrl(QStringLiteral("http://very-very-excellent-docs.lol"));
}

QUrl DummyResource::bugURL()
{
    return QUrl(QStringLiteral("file:///dev/null"));
}

QUrl DummyResource::donationURL()
{
    return QUrl(QStringLiteral("https://youtu.be/0o8XMlL8rqY"));
}

QUrl DummyResource::contributeURL()
{
    return QUrl(QStringLiteral("https://techbase.kde.org/Contribute"));
}

QVariant DummyResource::icon() const
{
    static const QVector<QVariant> icons = {QStringLiteral("device-notifier"), QStringLiteral("media-floppy"), QStringLiteral("drink-beer"), QStringLiteral("answer-correct")};
    return icons[m_type];
}

QString DummyResource::installedVersion() const
{
    return QStringLiteral("2.3");
}

QJsonArray DummyResource::licenses()
{
    return {QJsonObject{{QStringLiteral("name"), QStringLiteral("GPL")}, {QStringLiteral("url"), QStringLiteral("https://kde.org")}}};
}

QString DummyResource::longDescription()
{
    return QStringLiteral(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur ultricies consequat nulla, ut vulputate nulla ultricies ac. Suspendisse lacinia "
        "commodo lacus, non tristique mauris dictum vitae. Sed adipiscing augue nec nisi aliquet viverra. Etiam sit amet nulla in tellus consectetur feugiat. "
        "Cras in sem tortor. Fusce a nulla at justo accumsan gravida. Maecenas dui felis, lacinia at ornare sed, aliquam et purus. Sed ut sagittis lacus. "
        "Etiam dictum pharetra rhoncus. Suspendisse auctor orci ipsum. Pellentesque vitae urna nec felis consequat lobortis dictum in urna. Phasellus a mi ac "
        "leo adipiscing varius eget a felis. Cras magna augue, commodo sed placerat vel, tempus vel ligula. In feugiat quam quis est lobortis sed accumsan "
        "nunc malesuada. Mauris quis massa sit amet felis tempus suscipit a quis diam.\n\n"

        "Aenean quis nulla erat, vel sagittis sem. Praesent vitae mauris arcu. Cras porttitor, ante at scelerisque sodales, nibh felis consectetur orci, ut "
        "hendrerit urna urna non urna. Duis eu magna id mi scelerisque adipiscing. Aliquam sed quam in eros sodales accumsan. Phasellus tempus sagittis "
        "suscipit. Aliquam rutrum dictum justo ut viverra. Nulla felis sem, molestie sed scelerisque non, consequat vitae nulla. Aliquam ullamcorper malesuada "
        "mi, vel vestibulum magna vulputate eget. In hac habitasse platea dictumst. Cras sed lacus dui, vel semper sem. Aenean sodales porta leo vel "
        "fringilla.\n\n"

        "Ut tempus massa et urna porta non mollis metus ultricies. Duis nec nulla ac metus auctor porta id et mi. Mauris aliquam nibh a ligula malesuada sed "
        "tincidunt nibh varius. Sed felis metus, porta et adipiscing non, faucibus id leo. Donec ipsum nibh, hendrerit eget aliquam nec, tempor ut mauris. "
        "Suspendisse potenti. Vestibulum scelerisque adipiscing libero tristique eleifend. Donec quis tortor eget elit mollis iaculis ac sit amet nisi. Proin "
        "non massa sed nunc rutrum pellentesque. Sed dui lectus, laoreet sed condimentum id, commodo sed urna.\n\n"

        "Praesent tincidunt mattis massa mattis porta. Nullam posuere neque at mauris vestibulum vitae elementum leo sodales. Quisque condimentum lectus in "
        "libero luctus egestas. Fusce tempor neque ac dui tincidunt eget viverra quam suscipit. In hac habitasse platea dictumst. Etiam metus mi, adipiscing "
        "nec suscipit id, aliquet sed sem. Duis urna ligula, ornare sed vestibulum vel, molestie ac nisi. Morbi varius iaculis ligula. Nunc in augue leo, sit "
        "amet aliquam elit. Suspendisse rutrum sem diam. Proin eu orci nisl. Praesent porttitor dignissim est, id fermentum arcu venenatis vitae.\n\n"

        "Integer in sapien eget quam vulputate lobortis. Morbi nibh elit, elementum vitae vehicula sed, consequat nec erat. Donec placerat porttitor est ut "
        "dapibus. Fusce augue orci, dictum et convallis vel, blandit eu tortor. Phasellus non eros nulla. In iaculis nulla fermentum nulla gravida eu mattis "
        "purus consectetur. Integer dui nunc, sollicitudin ac tincidunt nec, hendrerit bibendum nunc. Proin sit amet augue ac velit egestas varius. Sed eu "
        "ante quis orci vestibulum sagittis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Phasellus vitae urna "
        "odio, at molestie leo. In convallis neque vel mi dictum convallis lobortis turpis sagittis.\n\n");
}

QString DummyResource::name() const
{
    return m_name;
}

QString DummyResource::origin() const
{
    return QStringLiteral("DummySource1");
}

QString DummyResource::packageName() const
{
    return m_name;
}

QString DummyResource::section()
{
    return QStringLiteral("dummy");
}

AbstractResource::State DummyResource::state()
{
    return m_state;
}

void DummyResource::fetchChangelog()
{
    Q_EMIT changelogFetched(longDescription());
}

void DummyResource::fetchScreenshots()
{
    Q_EMIT screenshotsFetched(m_screenshots);
}

void DummyResource::setState(AbstractResource::State state)
{
    m_state = state;
    Q_EMIT stateChanged();
}

void DummyResource::setAddons(const AddonList &addons)
{
    const auto addonsToInstall = addons.addonsToInstall();
    for (const QString &toInstall : addonsToInstall) {
        setAddonInstalled(toInstall, true);
    }
    const auto addonsToRemove = addons.addonsToRemove();
    for (const QString &toRemove : addonsToRemove) {
        setAddonInstalled(toRemove, false);
    }
}

void DummyResource::setAddonInstalled(const QString &addon, bool installed)
{
    for (auto &elem : m_addons) {
        if (elem.name() == addon) {
            elem.setInstalled(installed);
        }
    }
}

void DummyResource::invokeApplication() const
{
    QDesktopServices d;
    d.openUrl(QUrl(QStringLiteral("https://projects.kde.org/projects/extragear/sysadmin/muon")));
}

QUrl DummyResource::url() const
{
    return QUrl(QLatin1String("dummy://") + packageName().replace(QLatin1Char(' '), QLatin1Char('.')));
}

QStringList DummyResource::topObjects() const
{
    static QStringList s_objects{QStringLiteral("qrc:/qml/DummyTop.qml")};
    return s_objects;
}

#include "moc_DummyResource.cpp"
