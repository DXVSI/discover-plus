#!/bin/bash

# Fix all QString literal errors in QmlMaterial

# Fix pool.cpp
sed -i 's/QString("%1%2")/QString(QStringLiteral("%1%2"))/g' QmlMaterial/src/util/pool.cpp

# Fix token.cpp - use QLatin1String for macros
sed -i 's/return QM_VERSION;/return QLatin1String(QM_VERSION);/g' QmlMaterial/src/token/token.cpp
sed -i 's/return QM_ICON_FONT_URL;/return QLatin1String(QM_ICON_FONT_URL);/g' QmlMaterial/src/token/token.cpp
sed -i 's/return QM_ICON_FILL_0_FONT_URL;/return QLatin1String(QM_ICON_FILL_0_FONT_URL);/g' QmlMaterial/src/token/token.cpp
sed -i 's/return QM_ICON_FILL_1_FONT_URL;/return QLatin1String(QM_ICON_FILL_1_FONT_URL);/g' QmlMaterial/src/token/token.cpp

# Fix qml_util.cpp
sed -i 's/v\.property("name")/v.property(QStringLiteral("name"))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/v\.property("source")/v.property(QStringLiteral("source"))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/obj\.property("objectName")/obj.property(QStringLiteral("objectName"))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/obj\.property("parent")/obj.property(QStringLiteral("parent"))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/js\.property("destroy")/js.property(QStringLiteral("destroy"))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/doc\.toJson(QJsonDocument::Compact)/QString::fromUtf8(doc.toJson(QJsonDocument::Compact))/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/meta->indexOfSignal(signal_sig)/meta->indexOfSignal(signal_sig.constData())/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/QMetaObject::normalizedSignature(name\.toUtf8()\.constData()))/QMetaObject::normalizedSignature(name.toUtf8().constData()).constData())/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/QUrl url  = str;/QUrl url(str);/g' QmlMaterial/src/util/qml_util.cpp
sed -i "s/str\.split('/')/str.split(QChar('/'))/g" QmlMaterial/src/util/qml_util.cpp
sed -i 's/return obj\.toQObject()->metaObject()->className();/return QString::fromLatin1(obj.toQObject()->metaObject()->className());/g' QmlMaterial/src/util/qml_util.cpp
sed -i 's/return v\.metaType()\.name();/return QString::fromLatin1(v.metaType().name());/g' QmlMaterial/src/util/qml_util.cpp

echo "Fixed QmlMaterial QString issues"