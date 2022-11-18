#pragma once

#include <memory>

#include "db_pgsql_querye.h"
#include "db_mysql_querye.h"
#include "db_mssql_query.h"

namespace jara_lib {
    /*! Класс подключения к базе данных */
    class DbConnection {
    public:
        //! Тип состояния подключения к базе данных
        enum ConnectionType : ushort {
            //! Подключено к базе приложения
            APPLICATION_DB_CONNECTION,
            //! Подключено к мастер базе данных
            MASTER_DB_CONNECTION,
            //! Нет подключения к базе данных
            CONNECTION_REFUSED
        };

    public:
        /*! Конструктор принимает строку подключения (DbConnection) */
        DbConnection(const QString& connectionString = "",
                     DbType type = DbType::SQLITE);
        /*! Конструктор принимает параметры подключения (DbConnection) */
        DbConnection(const QString& hostname,
                     const QString& dbname,
                     const QString& username,
                     const QString& password,
                     unsigned short port = 0,
                     DbType type = DbType::SQLITE);
        /*! Деконструктор закрывает соединение, если оно открыто (DbConnection) */
        ~DbConnection();

        /*! Метод подключения к базе данных (DbConnection) */
        ConnectionType openConnection(bool master = false);

        //! Интерфейс запросов (DbConnection)
        std::shared_ptr<IDbCommand> Command = nullptr;

    private:
        /*! Выбор типа базы данных (DbConnection) */
        void initDbType();
        /*! Инициализация параметров подключения к базе данных (DbConnection) */
        void setDbParams();
        /*! Метод для парсинга строки подключения (DbConnection) */
        void connectionStringParser();

    public:
        /*! Выполнить запрос (DbConnection) */
        QSqlQuery proceedQuery(const QString&, bool master = false);
        QString getDbName() const { return _dbName; }
        DbType getDbType() const { return _dbType; }

    private:
        //! Имя сервера с СУБД (DbConnection)
        QString _dbHostName;
        //! Имя базы данных (DbConnection)
        QString _dbName;
        //! Имя главной базы в СУБД (DbConnection)
        QString _dbMasterName;
        //! Таблица с остальными параметрами подключения (DbConnection)
        QMap<QString, QString> _dbConnectionParameters;
        //! Имя пользователя для подключения к базы данных (DbConnection)
        QString _dbUserName;
        //! Пароль пользователя для подключения к бд (DbConnection)
        QString _dbUserPassword;
        //! Тип СУБД, который используется для бд (DbConnection)
        DbType _dbType;
        //! Номер порта для подключения к СУБД (DbConnection)
        unsigned short _dbPort;
        //! Строка подключения к базе данных (DbConnection)
        QString _dbConnectionString;
        //! Объект подключения к базе данных (DbConnection)
        QSqlDatabase _db;
        //! Состояние подключения к базе данны (DbConnection)
        ConnectionType _dbCurrentConnection;
    };
};

