#include "db_connection.h"

namespace jara_lib {
    /* Конструктор принимает строку подключения */
    DbConnection::DbConnection(const QString& connectionString, DbType type)
        : _dbType(type),
          _dbPort(0),
          _dbConnectionString(connectionString) {
        if (connectionString != "") {
            // Парсим строку подключения, если тип СУБД не sqlite
            if (type != DbType::SQLITE) {
                connectionStringParser();
            }
            // Задаем параметры подключения
            setDbParams();
        }
    }

    /* Конструктор принимает параметры подключения */
    DbConnection::DbConnection(
            const QString& hostname,
            const QString& dbname,
            const QString& username,
            const QString& password,
            unsigned short port,
            DbType type)
        : _dbHostName(hostname),
          _dbName(dbname),
          _dbUserName(username),
          _dbUserPassword(password),
          _dbType(type),
          _dbPort(port) {
        // Задаем параметры подключения
        setDbParams();
    }

    /* Деконструктор закрывает соединение, если оно открыто */
    DbConnection::~DbConnection() {
        if (_db.isOpen()) {
            _db.close();
        }
    }

    /* Метод подключения к базе данных */
    DbConnection::ConnectionType DbConnection::openConnection(bool master) {
        /*
         * Устанавливаем для какой базы данных создается соедиение.
         * Поскольку при первом обращении к базе данных её может не существовать,
         * то нужно обратиться к главной бд и осуществить подключение через неё.
         * Для этого в параметрах метода есть флаг schema, которая принимает
         * значение true, если нужно подключиться к главной базе, а не к базе приложения.
         * В том случае, если бд приложения уже существует, то просто подключяемся к ней.
         */

        // Получаем название базы, к которой будет осуществляться подключение
        if (master) {
            // Задаем подключение к базе мастера СУБД
            _db.setDatabaseName(_dbMasterName);
        }
        else {
            const QString &dbName = (_dbType == DbType::ODBC || _dbType == DbType::SQLITE)
                ? _dbConnectionString : _dbName;
            _db.setDatabaseName(dbName);
        }

        // Если есть подключение к базе данных
        if (_db.open()) {
            /*
             * в зависимости от того, к чему было подключение -
             * к мастер базе или базе приложения, записываем состояние типа подключения.
             */
            _dbCurrentConnection = (master) ? ConnectionType::MASTER_DB_CONNECTION
                                            : ConnectionType::APPLICATION_DB_CONNECTION;
        }
        else {
            // Если подключения не было, то заносим соответствующий тип
            _dbConnectionString = ConnectionType::CONNECTION_REFUSED;
        }
        // Возвращаем текущее состояние подключения
        return (_db.isOpen()) ? _dbCurrentConnection : ConnectionType::CONNECTION_REFUSED;
    }


    /* Выбор типа базы данных */
    void DbConnection::initDbType() {
        /*
         * В зависимости от типа СУБД, задаём используемый драйвер,
         * а также номер порта для подключения к БД.
         * QSqlDatabase::addDatabase("[DB TYPE]") подключает определенный типом
         * драйвер для подключения. Соответственно, для использования драйвер должен
         * быть предварительно установлен на компьютер. Исключение - sqlite.
         */
        switch (_dbType) {
        /*
         * В ветках для MySQL/PostgreSQL/MS SQL Server задаются параметры подключения:
         * 1. Тип драйвера СУБД --- addDatabase("[DB TYPE]")
         * 2. Имя главной базы данных в СУБД --- _dbSchemaParameters[0]
         * 3. Имя таблицы в базе --- _dbSchemaParameters[1]
         * 4. Название колонки в таблице --- _dbSchemaParameters[2]
         * 5. Номер порта для подключения --- _dbPort
         * Примечание: для подключения к sqlite не нужно ничего, кроме название базы данных,
         * поэтому нужно задать только строку подключения. При этом для sqlite не нужно
         * удостоверяться, что база данных существует - она будет создана автоматически при
         * первом обращении к ней с именем, указанным в строке подключения.
         */
            case DbType::MYSQL:
                _db = QSqlDatabase::addDatabase("QMYSQL");
                _dbMasterName = "INFORMATION_SCHEMA";
                _dbPort = (_dbPort == 0) ? 3306 : _dbPort;
                Command = std::make_shared<MysqlCommand>();
                break;
            case DbType::POSTGRES:
                _db = QSqlDatabase::addDatabase("QPSQL");
                _dbMasterName = "postgres";
                _dbPort = (_dbPort == 0) ? 5432 : _dbPort;
                Command = std::make_shared<PgsqlCommand>();
                break;
            case DbType::ODBC:
                _db = QSqlDatabase::addDatabase("QODBC3");
                _dbMasterName = _dbConnectionString;
                _dbMasterName.replace(_dbName, "master");
                _dbPort = (_dbPort == 0) ? 1433 : _dbPort;
                Command = std::make_shared<MssqlCommand>();
                break;
            case DbType::SQLITE:
                _db = QSqlDatabase::addDatabase("QSQLITE");
                break;
        }
        _dbCurrentConnection = ConnectionType::CONNECTION_REFUSED;
    }

    /* Метод чтобы задать параметры для подключения к базе данных */
    void DbConnection::setDbParams() {
        // Выбираем тип подключения к СУБД, драйвер и порт
        initDbType();

        // Задаем параметры подключения к базе данных
        _db.setHostName(_dbHostName);       // Название хоста с СУБД
        _db.setPort(_dbPort);               // Номер порта для подключения
        // Если подключаемся к sqlite или odbc, то используем строку подключения
        const QString &dbName = (_dbType == DbType::ODBC || _dbType == DbType::SQLITE)
            ? _dbConnectionString : _dbName;
        _db.setDatabaseName(dbName);        // Название базы данных приложения
        _db.setUserName(_dbUserName);       // Имя пользователя для подключения
        _db.setPassword(_dbUserPassword);   // Пароль для пользователя
    }

    /*
     * Метод парсинга строки подключения.
     * Нужно скорее для подключения к СУБД MS SQL Server (ODBC), однако для подключения
     * к остальным СУБД также может пригодиться, т.к. проще передать строку подключения
     * чем перечислять заданные параметры.
     */
    void DbConnection::connectionStringParser() {
        /*
         * Сначала делим всю строку подключения на части из отдельных заголовков,
         * типа 'Storage', 'User Id', 'Password' etc, и их значений.
         * В качестве разделителя берем точку-с-запятой (;).
         * Например, если была строка типа
         * Host=dbserver;User Id=some_user;Password=some_password, то
         * будет список частей из строк:
         * {Host=dbserver},{User Id=some_user},{Password=some_password}
         */
        const QVector<QStringRef> parts = _dbConnectionString.splitRef(";");

        // Проходим по всем разделенным частям строки подключения
        for (const QStringRef& part : parts) {
            /*
             * и каждую часть снова делим уже на отдельные составные части
             * с разделителем - знаком равно (=).
             * Например, часть Host=dbserver будет разделена на части
             * {Host} и {dbserver}, где {Host} - заголовок, который в данном
             * случае означает имя хоста, на котором находится СУБД и
             * {dbserver} - значение для заголовка, т.е. уже непосредственно имя хоста.
             */
            const QVector<QStringRef> nodes = part.split('=');

            // Набор должен состоять из двух элементов - заголовок и значение заголовка
            if (nodes.size() == 2) {
                // Далее просто получаем заголовки и сравниваем с различными вариантами
                QString lowerHeader = nodes.at(0).toString().toLower();
                if (lowerHeader == "host" ||
                    lowerHeader == "server" ||
                    lowerHeader == "data source") {
                    /*
                     * Строка с именем хоста может содержать также полный адрес с
                     * именем хоста, именем базы данных приложения и номер порта.
                     * Например, HOSTNAME\DBNAME,1430.
                     */
                    QStringRef hostName = nodes.at(1);
                    /*
                     * Если строка содержит обратный слеш, то она несет также название
                     * базы данных приложения.
                     */
                    if (hostName.indexOf("\\") > -1) {
                        // Делим строку с обратным слешом
                        QVector<QStringRef> hostValue = hostName.split("\\");
                        if (hostValue.size() == 2) {
                            // Получаем имя хоста с СУБД
                            _dbHostName = hostValue.at(0).toString();
                            // Смотрим, не указан ли также номер порта через запятую
                            if (hostValue.at(1).indexOf(",") > -1) {
                                // Если указан, то снова делим и получаем номер порта
                                QVector<QStringRef> dbNamePort =
                                        hostValue.at(1).split(",");
                                if (dbNamePort.size() == 2) {
                                    _dbName = dbNamePort.at(0).toString();
                                    _dbPort = dbNamePort.at(1).toUShort();
                                }
                            }
                            else {
                                // Если порт не указан, то просто закидываем название бд
                                _dbName = hostValue.at(1).toString();
                            }
                        }
                        // Для дальнейшей обработки строки обновляем подстроку с именем хоста
                        hostName = hostValue.at(0);
                    }

                    // Если имя хоста через запятую содержит номер порта
                    if (hostName.indexOf(",") > 0) {
                        // то получаем также номер порта и записываем в соответствующий параметр
                        QVector<QStringRef> hostValue = hostName.split(",");
                        if (hostValue.size() == 2) {
                            _dbHostName = hostValue.at(0).toString();
                            _dbPort = hostValue.at(1).toUShort();
                        }
                    }
                    else {
                        // Если имя хоста оказывается пустым, то записываем как localhost
                        _dbHostName = (hostName == "" || hostName == ".")
                                ? "localhost" : hostName.toString();
                    }

                }
                else if (lowerHeader == "user id" || lowerHeader == "uid") {
                    // Значение user id содержит имя пользователя бд
                    _dbUserName = nodes[1].toString();
                }
                else if (lowerHeader == "password" || lowerHeader == "pwd") {
                    // Пароль пользователя бд
                    _dbUserPassword = nodes[1].toString();
                }
                else if (lowerHeader == "initial catalog" || lowerHeader == "database") {
                    // название базы данных приложения
                    _dbName = nodes[1].toString();
                }
                else if (lowerHeader == "port") {
                    // порт подключения к базе данных
                    _dbPort = nodes[1].toUShort();
                }
                else {
                    /*
                     * Если есть какие-то другие параметры,
                     * записываем им в хэш-таблицу, где ключ - имя заголовка,
                     * а значение - это значение заголовка из строки подключения.
                     */
                    _dbConnectionParameters[lowerHeader] = nodes[1].toString();
                }
            }
        }

        /*
         * Небольшая проверка для подключения к СУБД MS Sql Server. В строке подключения
         * должен быть указан драйвер, через который осуществляется соединение.
         */
        if (!_dbConnectionParameters.contains("driver") && _dbType == DbType::ODBC) {
            // Добавляем вперед строки заголовок с типом драйвера
            _dbConnectionString.push_front("DRIVER={SQL Server};");
            // Добавляем информацию в словарь с параметрами
            _dbConnectionParameters["driver"] = "{SQL Server}";
            _db.setDatabaseName(_dbConnectionString);
        }
    }

    QSqlQuery DbConnection::proceedQuery(const QString &command, bool master) {
        if ((master && _dbCurrentConnection != MASTER_DB_CONNECTION) ||
            (!master && _dbCurrentConnection != APPLICATION_DB_CONNECTION) ||
            (_db.isOpen() && _dbCurrentConnection == CONNECTION_REFUSED)) {
            _db.close();
        }

        _dbCurrentConnection = openConnection(master);
        QSqlQuery query(_db);
        if (_dbCurrentConnection != CONNECTION_REFUSED && _db.isOpen() &&
            ((master && _dbCurrentConnection == MASTER_DB_CONNECTION) ||
             (!master && _dbCurrentConnection == APPLICATION_DB_CONNECTION))) {
            query.exec(command);
        }
        return query;
    }
};
