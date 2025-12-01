#pragma once
#include <QtCore/qtcoreexports.h>

#if defined(QML_MATERIAL_EXPORT)
#    define QML_MATERIAL_API Q_DECL_EXPORT
#else
#    define QML_MATERIAL_API Q_DECL_IMPORT
#endif