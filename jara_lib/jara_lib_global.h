#ifndef JARA_LIB_GLOBAL_H
#define JARA_LIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(JARA_LIB_LIBRARY)
#  define JARA_LIB_EXPORT Q_DECL_EXPORT
#else
#  define JARA_LIB_EXPORT Q_DECL_IMPORT
#endif

#endif // JARA_LIB_GLOBAL_H
