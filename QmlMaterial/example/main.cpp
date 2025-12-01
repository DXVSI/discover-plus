#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QLoggingCategory>

#include <QtQml/QQmlExtensionPlugin>

using namespace Qt::StringLiterals;

#ifdef QML_STATIC
Q_IMPORT_QML_PLUGIN(Qcm_MaterialPlugin)
#endif

int main(int argc, char* argv[]) {
    QGuiApplication gui_app(argc, argv);
    QCoreApplication::setApplicationName("qml_material");
    QCoreApplication::setApplicationVersion("0.0.1");


#ifdef __EMSCRIPTEN__
    QSurfaceFormat format;
    format.setMajorVersion(3);
#endif

    QQmlApplicationEngine engine;

    engine.addImportPath("qrc:/");
    engine.load(u"qrc:/main.qml"_s);

    return gui_app.exec();
}
