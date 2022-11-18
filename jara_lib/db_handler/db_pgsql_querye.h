#pragma once

#include "db_query_interface.h"

namespace jara_lib {
    class PgsqlCommand : public IDbCommand {
    public:
        PgsqlCommand() {
            ColumnTypes[ColumnType::INT_SERIAL] = "SERIAL NOT NULL";
            ColumnTypes[ColumnType::BIGINT_SERIAL] = "BIGSERIAL NOT NULL";
            ColumnTypes[ColumnType::STRING] = "VARCHAR NOT NULL";
            ColumnTypes[ColumnType::STRING_NULL] = "VARCHAR NULL";
        }

        QString createDatabase(const QString &dbName, bool = false) const override {
            QString queryCommand = "CREATE DATABASE \"" + dbName + "\"; ";
            return queryCommand;
        }

        QString checkDatabase(const QString &dbName) const override {
            QString queryCommand = "SELECT * FROM pg_database";
            queryCommand += " WHERE DATNAME";
            queryCommand += " = \'" + dbName + "\'";
            return queryCommand;
        }

        QString dropDatabase(const QString &dbName,
                             bool existsCheck = false) const override {
            // Формируем строку запроса на удаление базы
            QString queryCommand = "DROP DATABASE ";
            /*
             * Если поставлен флаг проверки существование базы,
             * добавляем подстроку в запроса
             */
            queryCommand += (existsCheck) ? "IF EXISTS \"" : "\"";
            // Записываем название базы
            queryCommand += dbName + "\";";
            return queryCommand;
        }

        QString createTable(const QString&, const DbTable &table,
                            bool existsCheck = false) const override {
            QString queryCommand = "CREATE TABLE ";
            queryCommand += (existsCheck) ? " IF NOT EXISTS " : "";
            queryCommand += "\"" + table->getModelName() + "\" (";

            // Добавляем колонки
            queryCommand += prepareColumns(table->getTableColumns());

            // Добавляем запрос на создание первичного ключа
            queryCommand += primaryConstraint(table->getPkColumn());

            queryCommand += "); ";
            return queryCommand;
        }

        QString checkTable(const QString&,
                           const DbTable &table) const override {
            QString queryCommand = "SELECT COUNT(*) FROM PG_TABLES";
            queryCommand += " WHERE SCHEMANAME = \'public\' AND";
            queryCommand += " TABLENAME = \'";
            queryCommand += table->getModelName() + "\'; ";
            return queryCommand;
        }

        QString dropTable(const QString&, const DbTable &table,
                          bool existsCheck = false) const override {

            // Создаём строку для удаления таблицы
            QString queryCommand = "DROP TABLE ";
            // Добавляем проверку существования таблицы, если поставлен флаг
            queryCommand += (existsCheck) ? "IF EXISTS \"" : "\"";
            /*
             * Добавляем имя базы, в которой будет создана таблица
             * и через точку имя создаемой таблицы.
             */
            queryCommand += table->getModelName() + "\"; ";

            return queryCommand;
        }

        QString prepareColumn(const DbColumn &column,
                              bool cleared = false) const override {

            QString queryCommand = "\"" + column->getModelName() + "\" ";
            queryCommand +=
                (!cleared) ? ColumnTypes[column->getModelType()] + " " : "";

            return queryCommand;
        }

        QString alterAddColumn(const QString&,
                               const DbColumn &column,
                               bool existsCheck = false) const override {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE \"";
            // Вносим имя редактируемой таблицы
            queryCommand += column->getTable()->getModelName() + "\" ADD ";
            // Если флаг проверки поставлен
            if (existsCheck) {
                // добавляем строку на проверку существования колонки
                queryCommand += "IF NOT EXISTS ";
            }
            // Вносим имя и тип колонки
            queryCommand += prepareColumn(column) + "; ";

            return queryCommand;
        }

        QString alterModifyColumn(const QString&,
                                  const DbColumn &column,
                                  bool = false) const override {

            const QString& columnType = ColumnTypes[column->getModelType()];
            QStringRef type = columnType.midRef(0, columnType.indexOf(' '));
            QStringRef nullable = columnType.midRef(columnType.indexOf(' '));

            /*
             * В postgres нельзя при изменении колонки задавать отдельный тип
             * данных и нулевой признак поля. Поэтому нужно делать два отдельных
             * запроса для изменения типа и нулевого признака.
             */
            QString queryCommand = "ALTER TABLE \"";
            queryCommand += column->getTable()->getModelName() + "\" ALTER COLUMN ";
            queryCommand += "\"" + column->getModelName() + "\" ";
            queryCommand += "TYPE " + type + "; ";

            queryCommand += "ALTER TABLE \"";
            queryCommand += column->getTable()->getModelName() + "\" ALTER COLUMN ";
            queryCommand += "\"" + column->getModelName() + "\" ";
            queryCommand += (nullable.contains("NOT NULL"))
                    ? "SET NOT NULL; "
                    : "DROP NOT NULL; ";

            return queryCommand;
        }

        QString alterDropColumn(const QString&,
                                const DbColumn &column,
                                bool existsCheck = false) const override {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE \"";
            // Вносим имя редактируемой таблицы
            queryCommand += column->getTable()->getModelName() + "\" DROP COLUMN ";
            // Если флаг проверки поставлен
            if (existsCheck) {
                // добавляем строку на проверку существования колонки
                queryCommand += "IF EXISTS ";
            }
            // Вносим имя колонки
            queryCommand += prepareColumn(column, false) + "; ";

            return queryCommand;
        }

        QString preparePrimaryKey(const DbColumn &column) const override {
            QString preparedKey = "\"PK_" + column->getTable()->getModelName();
            preparedKey += "_" + column->getModelName() + "\"";
            return preparedKey;
        }

        QString primaryConstraint(const DbColumn &column) const override {

            QString queryCommand = "CONSTRAINT ";
            queryCommand += preparePrimaryKey(column);
            queryCommand += " PRIMARY KEY (\"" + column->getModelName() + "\") ";

            return queryCommand;
        }

        QString prepareForeignKey(const DbTable &fTable,
                                  const DbTable &pTable) const override {
            QString queryCommand = "\"FK_";
            queryCommand += fTable->getModelName() + "_";
            queryCommand += pTable->getModelName() + "\"";
            return queryCommand;
        }

        QString foreignConstraint(const DbColumn &foreign,
                                  const DbColumn &primary) const override {

            QString queryCommand =
                prepareForeignKey(foreign->getTable(), primary->getTable()) + " ";
            queryCommand += "FOREIGN KEY (\"" + foreign->getModelName();
            queryCommand += "\") REFERENCES \"";
            queryCommand += primary->getTable()->getModelName();
            queryCommand += "\"(\"" + primary->getModelName() + "\") ";

            return queryCommand;
        }

        QString checkForeignKey(const QString &dbName,
                                const DbTable &fTable,
                                const DbTable &pTable) const override {
            QString foreignKey = prepareForeignKey(fTable, pTable);
            QString queryCommand = "SELECT COUNT(*) ";
            queryCommand += "FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS ";
            queryCommand += "WHERE CONSTRAINT_CATALOG = \'" + dbName + "\' ";
            queryCommand += "AND CONSTRAINT_NAME = \'";
            queryCommand += foreignKey.replace("\"", "") + "\' ";
            queryCommand += "AND CONSTRAINT_TYPE = \'FOREIGN KEY\'; ";

            return queryCommand;
        }

        QString alterAddConstraint(const QString&,
                                   const DbColumn &foreign,
                                   const DbColumn &primary) const override {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE \"";
            // Вносим имя редактируемой таблицы
            queryCommand += foreign->getTable()->getModelName();
            queryCommand += "\" ADD CONSTRAINT ";
            // Вносим имя и тип колонки
            queryCommand += foreignConstraint(foreign, primary) + "; ";

            return queryCommand;
        }

        QString alterDropConstraint(const QString&,
                                    const DbColumn &foreign,
                                    const DbColumn &primary) const override {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE \"";
            // Вносим имя редактируемой таблицы
            queryCommand += foreign->getTable()->getModelName();
            queryCommand += "\" DROP CONSTRAINT ";
            // Вносим имя и тип колонки
            queryCommand += prepareForeignKey(
                foreign->getTable(), primary->getTable()) + "; ";

            return queryCommand;
        }

        QString getTableColumns(const QString&,
                                const DbTable &table) const override {
            QString queryCommand = "SELECT COLUMN_NAME, DATA_TYPE, ";
            queryCommand += "IS_NULLABLE, CHARACTER_OCTET_LENGTH, COLUMN_DEFAULT ";
            queryCommand += "FROM INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += table->getModelName() + "\'";
            return queryCommand;
        }

        QString getTableColumn(const QString&,
                               const DbColumn &column) const override {
            QString queryCommand = "SELECT COLUMN_NAME, DATA_TYPE, ";
            queryCommand += "IS_NULLABLE, COLUMN_DEFAULT ";
            queryCommand += "FROM INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += column->getTable()->getModelName() + "\' AND ";
            queryCommand += "COLUMN_NAME = \'";
            queryCommand += column->getModelName() + "\'";
            return queryCommand;
        }
    };
};
