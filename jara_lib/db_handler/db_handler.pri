DEFINES += JARA_LIB_LIBRARY

HEADERS += \
    $$PWD/db_connection.h \
    $$PWD/db_model_interface.h \
    $$PWD/db_mssql_query.h \
    $$PWD/db_mysql_querye.h \
    $$PWD/db_pgsql_querye.h \
    $$PWD/db_query_interface.h

SOURCES += \
    $$PWD/db_connection.cpp \
    $$PWD/db_model_interface.cpp
