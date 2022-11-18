#pragma once

#include <QStack>
#include <QDebug>
#include <QVariant>
#include "table_model.h"
#include "column_model.h"
#include "db_handler/db_connection.h"

namespace jara_lib {
    class ModelContext : public IModelContext {
    public:
        /*! Контекст должен принять строку подключения к базе (ModelContext) */
        explicit ModelContext(const DbConnection &connection)
            : _connection(connection) { }

        /*! Регистрация таблицы, связанной с контекстом (ModelContext) */
        void registerTable(const DbTable &table) override {
            // Находим таблицу с тем же названием среди привязанных к
            // контексту таблиц
            QHash<DbTable, QVector<DbTable>>::iterator _found = _tables.end();
            for (auto it = _tables.begin(); it != _tables.end(); ++it) {
                if (it.key()->getModelName() == table->getModelName()) {
                    _found = it;
                    break;
                }
            }
            // Если таблицы нет в коллекции данного контекста
            if (_found == _tables.end()) {
                // то связываем таблицу с данным контекстом
                _tables[table] = QVector<DbTable>();
            }
            else {
                // или закидываем в соответствующую таблицу
                _found.value().append(table);
            }
            // В привязанной таблице регистрируем данный контекст
            table->registerContext(this);
        }

        DbType getDbType() const override
        { return _connection.getDbType(); }

        QSqlQuery proceedExpression(
            const IExpressionHandler &expression) override {
            const ExpressionNodes &nodes = expression.getExpressionNodes();

            if (nodes.count()) {
                QString expression = "";

                for (QueryClause clause = QueryClause::SELECT;
                     clause <= QueryClause::DESC;
                     clause = QueryClause(ushort(clause) + 1)) {

                    expression += _connection.Command->
                        makeExpressionClause(clause, nodes[clause]);
                }

                return _connection.proceedQuery(expression.trimmed());
            }

            return QSqlQuery();
        }

    private:
        /*! Метод проверки существования базы данных (ModelContext) */
        bool databaseExists() {
            // Создаем строку запроса для проверки существования
            QString command = _connection.Command->checkDatabase(
                _connection.getDbName());
            // Выполняем запрос, подключившись к главной базе данных
            QSqlQuery query = _connection.proceedQuery(command, true);
            return query.first();
        }

        /*! Метод для создания базы данных (ModelContext) */
        bool databaseCreate() {
            // Создаем строку запроса на создание базы приложения
            QString command = _connection.Command->createDatabase(
                _connection.getDbName());
            // Выполняем запрос от имени главной базы в СУБД
            _connection.proceedQuery(command, true);
            // Возвращаем результат, проверяя наличие базы
            return (databaseExists());
        }

        /*! Метод для проверки существования таблицы в базе (ModelContext) */
        bool tableExists(const DbTable &table) {
            const QString &dbName = _connection.getDbName();

            // Создаем строку для проверки существования таблицы
            QString command =
                _connection.Command->checkTable(dbName, table);

            // Добавляем строку в объект запроса
            QSqlQuery query = _connection.proceedQuery(command);
            // Если запроса вернул результат
            if (query.first()) {
                /*
                 * Проверяем результат, если был возвращен ноль,
                 * то таблицы в базе не существует. Другое значение
                 * будет означать существования таблицы в базе.
                 */
                return query.value(0).toBool();
            }
            return false;
        }

        bool columnExists(const DbColumn &column) {
            const QString &dbName = _connection.getDbName();

            QString command = _connection.Command->
                getTableColumn(dbName, column);

            QSqlQuery query = _connection.proceedQuery(command);

            return query.first();
        }

        bool columnMatched(const DbColumn &column) {
            const QString &dbName = _connection.getDbName();

            QString command = _connection.Command->
                getTableColumn(dbName, column);

            QSqlQuery query = _connection.proceedQuery(command);

            if (query.first()) {
                ColumnInfo columnInfo;
                columnInfo.columnName = query.value(0).toString();

                QString columnType = query.value(1).toString().toLower();
                bool isNullable = query.value(2).toString().toLower() == "yes";
                QString is_identity = (!query.value(3).isNull()) ?
                    query.value(3).toString().toLower() : "";

                bool autoIncrement = false;
                ColumnType type = ColumnType::STRING_NULL;

                switch (_connection.getDbType()) {
                    case DbType::MYSQL:
                        if (is_identity == "auto_increment") {
                            autoIncrement = true;
                        }

                        if (columnType == "int") {
                            type = (isNullable) ? ColumnType::INT_NULL
                                                : (autoIncrement) ? ColumnType::INT_SERIAL
                                                                  : ColumnType::INT;

                        }
                        else if (columnType == "bigint") {
                            type = (isNullable) ? ColumnType::BIGINT_NULL
                                                : (autoIncrement) ? ColumnType::BIGINT_SERIAL
                                                                  : ColumnType::BIGINT;
                        }
                        else if (columnType == "text" ||
                                 columnType.contains("varchar")) {
                            type = (isNullable) ? ColumnType::STRING_NULL
                                                : ColumnType::STRING;
                        }

                        break;
                    case DbType::ODBC:
                        if (is_identity == "1") {
                            autoIncrement = true;
                        }

                        if (columnType == "int") {
                            type = (isNullable) ? ColumnType::INT_NULL
                                                : (autoIncrement) ? ColumnType::INT_SERIAL
                                                                  : ColumnType::INT;
                        }
                        else if (columnType == "bigint") {
                            type = (isNullable) ? ColumnType::BIGINT_NULL
                                                : (autoIncrement) ? ColumnType::BIGINT_SERIAL
                                                                  : ColumnType::BIGINT;
                        }
                        else if (columnType.contains("varchar")) {
                            type = (isNullable) ? ColumnType::STRING_NULL
                                                : ColumnType::STRING;
                        }

                        break;

                    case DbType::POSTGRES:
                        if (is_identity == "auto_increment") {
                            autoIncrement = true;
                        }

                        if (is_identity.contains("nextval(") &&
                            is_identity.contains("::regclass")) {
                            autoIncrement = true;
                        }

                        if (columnType == "integer") {
                            type = (isNullable) ? ColumnType::INT_NULL
                                                : (autoIncrement) ? ColumnType::INT_SERIAL
                                                                  : ColumnType::INT;
                        }
                        else if (columnType == "biginteger") {
                            type = (isNullable) ? ColumnType::BIGINT_NULL
                                                : (autoIncrement) ? ColumnType::BIGINT_SERIAL
                                                                  : ColumnType::BIGINT;
                        }
                        else if (columnType == "character varying") {
                            type = (isNullable) ? ColumnType::STRING_NULL
                                                : ColumnType::STRING;
                        }

                        break;
                    case DbType::SQLITE:
                        break;
                }
                columnInfo.columnType = type;

                return (columnInfo.columnName == column->getModelName() &&
                        columnInfo.columnType == column->getModelType());
            }
            return false;
        }

        bool foreignKeyExists(const DbTable &fTable, const DbTable &pTable) {
            const QString &dbName = _connection.getDbName();

            QString command = _connection.Command->
                checkForeignKey(dbName, fTable, pTable);

            QSqlQuery query = _connection.proceedQuery(command);

            if (query.first()) {
                return query.value(0).toBool();
            }

            return false;
        }

        /*! Метод для создания таблиц (ModelContext) */
        void createTable(DbTable table) {
            /* Проверяем существует ли данная таблица в базе */
            if (tableExists(table)) {
                // Если существует, то ничего не делаем
                return;
            }

            const QString &dbName = _connection.getDbName();
            // Создаем текстовый запрос на создание таблица
            QString command =
                _connection.Command->createTable(dbName, table);

            // Проходим по коллекции вторичных ключей данной таблицы
            for (const QPair<DbTable, DbColumn> &item :
                 as_const(table->getFkColumns())) {
                /*
                 * Таблица со связанным вторичным ключом не может быть создана,
                 * пока не создана таблица, с которым связан вторичный ключ.
                 * Поэтому идём к главной таблице, с который был связан ключ,
                 * и создаем сначала её. В ней также будут проверяться наличие связей
                 * с другими таблицами.
                 */
                // Создаём связанную таблицу, если она ещё не создана
                createTable(item.first);
                // Добавляем подстроку для создачния записи о вторичном ключе
                command += _connection.Command->alterAddConstraint(
                    dbName, item.second, item.first->getPkColumn());
            }

            // Выполняем запрос на создание таблицы
            _connection.proceedQuery(command);
        }

    public:
        /*!
         *  Метод для приведения структуры базы приложения в соответствие с тем,
         *  как реализована структура данных моделей в коде (ModelContext)
         */
        void migrate() override {
            // Строка основного запроса
            QString command = "";
            // Дополнения к запросу в случае, если нужно добавить вторичные ключи
            /*
             * Пояснение: все операции со вторичными ключами желательно выполнять
             * в последнюю очередь, т.к. они сильно завязаны на существующих в базе
             * таблицам. Т.е., например, невозможно
             */
            QString foreignKeys = "";

            // Название базы приложений
            const QString &dbName = _connection.getDbName();

            const QList<DbTable> &tables = _tables.keys();
            // Проходим по коллекции таблиц контекста
            for (const DbTable &table : qAsConst(tables)) {
                // Проверяем существует ли таблица в базе
                if (tableExists(table)) {
                    // Если таблица существует, проходим по коллекции колонок
                    for (const DbColumn &column :
                         as_const(table->getTableColumns())) {
                        // Если колонка существует
                        if (columnExists(column)) {
                            // Проверяем совпадает ли её описание в базе
                            // с реализацией в коде
                            if (!columnMatched(column)) {
                                // и если не совпадает, приводим в соответствии с тем,
                                // как таблица реализована в коде
                                command += _connection.Command->
                                    alterModifyColumn(dbName, column);
                            }
                        }
                        else {
                            // Если колонка не существует, необходимо поменять таблицу,
                            // добавив туда нужную колонку
                            command += _connection.Command->
                                alterAddColumn(dbName, column);
                        }
                    }
                }
                else {
                    // Если таблицы не существует, добавляем таблицу
                    command += _connection.Command->createTable(dbName, table);
                }

                // Проходим по коллекции вторичных ключей для текущей таблицы
                for (const QPair<DbTable, DbColumn> &item :
                     as_const(table->getFkColumns())) {

                    // Подготавливаем название вторичного ключа
                    const QString &fKey = _connection.Command->
                        prepareForeignKey(table, item.first);

                    // Если вторичный ключ не существует в базе и если мы уже не записывали
                    // его в строку запроса для добавления вторичных ключей, то
                    // создаем подстроку для создания данного ключа
                    if (!foreignKeyExists(table, item.first) &&
                        !foreignKeys.contains(fKey)) {
                        foreignKeys += _connection.Command->alterAddConstraint(
                            dbName, item.second, item.first->getPkColumn());
                    }
                }
            }

            // Добавляем строку для создания вторичных ключей в основную строку запроса
            command += foreignKeys;
            // Если строка запроса не пуста, то пытаемся выполнить данный запрос
            if (!command.isEmpty()) {
                _connection.proceedQuery(command);
            }
        }

        /*! Метод инициализации базы данных (ModelContext) */
        void dbInit() override {
            // Проверям существование базы данных приложения
            if (databaseExists()) {
                qDebug() << "The database"
                         << _connection.getDbName() << "exists";
                migrate();
            }
            else {
                qDebug() << "The database" << _connection.getDbName()
                         << "doesn't exist";
                // Если базы не существует, то пытаемся создать её
                if (databaseCreate()) {
                    qDebug() << "The database" << _connection.getDbName()
                             << "is created";
                    // затем проходим по коллекции таблиц
                    const QList<DbTable> &tables = _tables.keys();
                    for (const DbTable &table : qAsConst(tables)) {
                        // и создаём их в базе
                        createTable(table);
                        if (tableExists(table)) {
                            qDebug() << "The table" << table->getModelName()
                                     << "is created";
                        }
                        else {
                            throw "Couldn\'t create the table " + table->getModelName();
                        }
                    }
                }
            }
        }

    protected:
        /*
         * Здесь коллекция таблиц, где значение, которое представляет
         * из себя коллекцию того же типа, что и ключ нужно для выстраивания
         * структуры таблицы и хранения данных из этой таблицы. Т.е. ключ
         * нужен скорее для связи данной таблицы с контекстом, тогда как
         * коллекция таблиц в значении нужна для хранения коллекции таблиц.
         */
        /*! Коллекция таблиц, связанных с контекстом (ModelContext) */
        QHash<DbTable, QVector<DbTable>> _tables;

        /*! Объект подключения к базе данных (ModelContext) */
        DbConnection _connection;
    };
};
