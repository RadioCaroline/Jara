QT -= gui
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-jara_lib-Desktop_Qt_5_15_2_MinGW_64_bit-Debug/release/ -ljara_lib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-jara_lib-Desktop_Qt_5_15_2_MinGW_64_bit-Debug/debug/ -ljara_lib

INCLUDEPATH += $$PWD/../jara_lib
DEPENDPATH += $$PWD/../jara_lib

HEADERS += \
    application_context.h \
    employee_table.h
