QT -= gui
QT += sql

TEMPLATE = lib
DEFINES += JARA_LIB_LIBRARY

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES +=

HEADERS += \
    jara_lib_global.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

include("model_handler/model_handler.pri")
include("db_handler/db_handler.pri")
include("utilities/utilities.pri")
