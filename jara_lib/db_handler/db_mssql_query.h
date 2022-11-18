#pragma once

#include "db_query_interface.h"

namespace jara_lib {
    /*! Класс для создания запросов в СУБД */
    class MssqlCommand : public IDbCommand {
    public:
        MssqlCommand() {
            // Заполняем общие типы для СУБД MS SQL Server (ODBC)
            ColumnTypes[ColumnType::INT_SERIAL] = "INT IDENTITY(1,1) NOT NULL";
            ColumnTypes[ColumnType::BIGINT_SERIAL] = "BIGINT IDENTITY(1,1) NOT NULL";
            ColumnTypes[ColumnType::STRING] = "NVARCHAR(MAX) NOT NULL";
            ColumnTypes[ColumnType::STRING_NULL] = "NVARCHAR(MAX) NULL";
        }

        QString wrapQuery(const QString &query) const
        { return " BEGIN " + query + " END; "; };

        /*!
         *  Строка запроса для создания базы данных (MssqlCommand);
         *  {dbName} - имя создаваемой базы данных;
         *  {existsCheck} - необязательный параметр проверки существования базы;
         */
        QString createDatabase(const QString &dbName,
                               bool existsCheck = false) const override {
            // Формируем строку запроса
            QString queryCommand = "";

            // Проверяем флаг проверки существования базы
            if (existsCheck) {
                /*
                 * Если флаг проставлен, то добавляем в запрос условие
                 * проверки существования базы данных
                 */
                queryCommand += "IF NOT EXISTS (SELECT * FROM SYS.DATABASES ";
                queryCommand += "WHERE NAME = \'" + dbName + "\')";
            }
            queryCommand += "CREATE DATABASE " + dbName;

            return wrapQuery(queryCommand);
        }

        /*!
         *  Проверка существования базы данных в СУБД (MssqlCommand);
         *  {dbName} - имя проверяемой базы данных;
         */
        QString checkDatabase(const QString &dbName) const override {
            // Создаем запрос на существование записи о таблице в
            // таблице СУБД MS SQL Server SYS.DATABASES
            QString queryCommand = "SELECT * FROM SYS.DATABASES";
            queryCommand += " WHERE NAME";
            // записываем название интересующей базы
            queryCommand += " = \'" + dbName + "\'";
            return wrapQuery(queryCommand);
        }

        /*!
         *  Строка запроса для удаления базы данных (MssqlCommand);
         *  {dbName} - имя удаляемой базы данных;
         *  {existsCheck} - необязательный параметр проверки существования базы;
         */
        QString dropDatabase(const QString &dbName,
                             bool existsCheck = false) const override {
            QString queryCommand = "";

            // Проверяем флаг проверки существования базы
            if (existsCheck) {
                /*
                 * Если флаг проставлен, то добавляем в запрос условие
                 * проверки существования базы данных
                 */
                queryCommand += "IF NOT (SELECT * FROM SYS.DATABASES ";
                queryCommand += "WHERE NAME = \'" + dbName + "\') ";
            }
            queryCommand += "DROP DATABASE " + dbName;

            return wrapQuery(queryCommand);
        }

        /*!
         *  Строка запроса для таблицы в базе (MssqlCommand);
         *  {tbName} - имя создаваемой таблицы;
         *  {dbName} - имя базы данных, в которой создаётся таблица;
         *  {existsCheck} - необязательный параметр проверки существования таблицы;
         */
        QString createTable(const QString &dbName,
                            const DbTable &table,
                            bool existsCheck = false) const override {
            // Создаем строку запроса с указанием базы, в которой
            // будет создаваться таблица
            QString queryCommand = "USE " + dbName + " ";
            // Если проставлен флаг проверки существования таблица
            if (existsCheck) {
                // заносим подстроку для проведения данной проверки
                queryCommand += "IF NOT EXISTS (SELECT * FROM ";
                queryCommand += "sysobjects WHERE NAME = \'" + table->getModelName();
                queryCommand += "\' AND XTYPE = 'U') ";
            }
            // Добавляем подстроку для создания таблицы
            // На момент создания строки неизвестно какие колонки будут созданы,
            // поэтому просто добавляем тэг #COLUMN_ENUM#
            queryCommand += "CREATE TABLE " + table->getModelName() + " (";

            // Добавляем колонки
            queryCommand += prepareColumns(table->getTableColumns());

            // Добавляем запрос на создание первичного ключа
            queryCommand += primaryConstraint(table->getPkColumn());

            queryCommand += "); ";

            return wrapQuery(queryCommand);
        }

        /*!
         *  Проверка существование таблицы в базе данных (MssqlCommand);
         *  {tbName} - имя проверяемой таблицы;
         *  {dbName} - имя базы данных, в которой должна быть таблица;
         */
        QString checkTable(const QString &dbName,
                           const DbTable &table) const override {
            // Создаётся строка запроса для проверки существования базы
            QString queryCommand = "SELECT COUNT(*)";
            // Проверка осуществляется через запрос в системную таблицу
            queryCommand += " FROM " + dbName + ".SYS.TABLES ";
            queryCommand += " WHERE NAME = \'" + table->getModelName() + "\'";
            return wrapQuery(queryCommand);
        }

        /*!
         *  Строка запроса для удаления таблицы в базе (MssqlCommand);
         *  {tbName} - имя удаляемой таблицы;
         *  {dbName} - имя базы данных, в которой удаляется таблица;
         *  {existsCheck} - необязательный параметр проверки существования таблицы;
         */
        QString dropTable(const QString &dbName,
                          const DbTable &table,
                          bool existsCheck = false) const override {
            // Создаем строку запроса с указанием базы, в которой
            // будет удаляться таблица
            QString queryCommand = "USE " + dbName + " ";
            // Если проставлен флаг проверки существования таблицы
            if (existsCheck) {
                // заносим подстроку для проведения данной проверки
                queryCommand += "IF EXISTS (SELECT * FROM ";
                queryCommand += "sysobjects WHERE NAME = \'";
                queryCommand += table->getModelName();
                queryCommand += "\' AND XTYPE = 'U') ";
            }
            // Добавляем подстроку для удаления таблицы
            queryCommand += "DROP TABLE " + table->getModelName() + "; ";
            return wrapQuery(queryCommand);
        }

        /*!
         *  Изменение таблицы: добавление колонки (MssqlCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя добавляемой колонки;
         *  {type} - тип добавляемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        QString alterAddColumn(const QString &dbName,
                               const DbColumn &column,
                               bool existsCheck = false) const override {

            // Создаем строку запроса. Указываем название редактируемой базы
            QString queryCommand = "USE " + dbName;
            // Если проставлен флаг проверки существования колонки
            if (existsCheck) {
                // заносим подстроку для проведения данной проверки
                queryCommand += " IF NOT EXISTS (SELECT 1 FROM SYS.COLUMNS ";
                queryCommand += "WHERE SYS.COLUMNS.NAME = \'";
                queryCommand += column->getTable()->getModelName() + "\') ";
            }
            // Добавляем подстроку для добавления колонки в таблицу
            queryCommand += " BEGIN ALTER TABLE ";
            queryCommand += column->getTable()->getModelName();
            queryCommand += " ADD " + column->getModelName() + " ";
            queryCommand += ColumnTypes[column->getModelType()] + " END;";

            return wrapQuery(queryCommand);
        }

        /*!
         *  Изменение таблицы: изменение колонки (MssqlCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя изменяемой колонки;
         *  {type} - тип изменяемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        QString alterModifyColumn(const QString &dbName,
                                  const DbColumn &column,
                                  bool existsCheck = false) const override {
            // Создаем строку запроса. Указываем название редактируемой базы
            QString queryCommand = "USE " + dbName;
            // Если проставлен флаг проверки существования колонки
            if (existsCheck) {
                // заносим подстроку для проведения данной проверки
                queryCommand += " IF EXISTS (SELECT 1 FROM SYS.COLUMNS ";
                queryCommand += "WHERE SYS.COLUMNS.NAME = \'";
                queryCommand += column->getTable()->getModelName() + "\') ";
            }
            // Добавляем подстроку для изменения колонки в таблице
            queryCommand += " BEGIN ALTER TABLE " + column->getTable()->getModelName();
            queryCommand += " ALTER COLUMN " + column->getModelName() + " ";
            queryCommand += ColumnTypes[column->getModelType()] + " END;";

            return wrapQuery(queryCommand);
        }

        /*!
         *  Изменение таблицы: удаление колонки (MssqlCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя удаляемой колонки;
         *  {type} - тип удаляемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        QString alterDropColumn(const QString &dbName,
                                const DbColumn &column,
                                bool existsCheck = false) const override {
            // Создаем строку запроса. Указываем название редактируемой базы
            QString queryCommand = "USE " + dbName;
            // Если проставлен флаг проверки существования колонки
            if (existsCheck) {
                // заносим подстроку для проведения данной проверки
                queryCommand += " IF EXISTS (SELECT 1 FROM SYS.COLUMNS ";
                queryCommand += "WHERE SYS.COLUMNS.NAME = \'";
                queryCommand += column->getTable()->getModelName() + "\')";
            }
            // Добавляем подстроку для удаления колонки в таблице
            queryCommand += " BEGIN ALTER TABLE " + column->getTable()->getModelName();
            queryCommand += " DROP COLUMN " + column->getModelName() + " END;";

            return wrapQuery(queryCommand);
        }

        QString checkForeignKey(const QString &dbName,
                                const DbTable &fTable,
                                const DbTable &pTable) const override {
            QString foreignKey = prepareForeignKey(fTable, pTable);
            QString queryCommand = "USE " + dbName;
            queryCommand += " SELECT COUNT(*) ";
            queryCommand += "FROM SYS.FOREIGN_KEYS ";
            queryCommand += "WHERE SYS.FOREIGN_KEYS.NAME = \'" + foreignKey + "\'; ";
            return wrapQuery(queryCommand);
        }

        QString alterAddConstraint(const QString &dbName,
                                   const DbColumn &foreign,
                                   const DbColumn &primary) const override {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "USE " + dbName;

            queryCommand += " IF NOT EXISTS(SELECT * FROM SYS.FOREIGN_KEYS ";
            queryCommand += "WHERE SYS.FOREIGN_KEYS.NAME = \'";
            queryCommand += prepareForeignKey(
                foreign->getTable(), primary->getTable()) + "\')";

            // Вносим имя редактируемой таблицы
            QString alterCommand = " ALTER TABLE " + foreign->getTable()->getModelName();
            alterCommand += " ADD CONSTRAINT ";
            // Вносим имя и тип колонки
            alterCommand += foreignConstraint(foreign, primary) + "; ";

            queryCommand += alterCommand;

            return wrapQuery(queryCommand);
        }

        QString getTableColumns(const QString &dbName,
                                const DbTable &table) const override {
            QString queryCommand = "USE " + dbName + " ";
            queryCommand += "SELECT COLUMN_NAME, DATA_TYPE, ";
            queryCommand += "IS_NULLABLE, CHARACTER_OCTET_LENGTH, ";
            queryCommand += "CONVERT(int, (COLUMNPROPERTY(object_id(TABLE_SCHEMA+'.'";
            queryCommand += "+TABLE_NAME), COLUMN_NAME, 'IsIdentity')))";
            queryCommand += " AS IS_IDENTITY ";
            queryCommand += "FROM " + dbName + ".INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += table->getModelName() + "\'";
            return wrapQuery(queryCommand);
        }

        QString getTableColumn(const QString &dbName,
                               const DbColumn &column) const override {
            QString queryCommand = "USE " + dbName + " ";
            queryCommand += "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE, ";
            queryCommand += "CONVERT(int, (COLUMNPROPERTY(object_id(TABLE_SCHEMA+'.'";
            queryCommand += "+TABLE_NAME), COLUMN_NAME, 'IsIdentity')))";
            queryCommand += " AS IS_IDENTITY ";
            queryCommand += "FROM INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += column->getTable()->getModelName() + "\' AND ";
            queryCommand += "COLUMN_NAME = \'";
            queryCommand += column->getModelName() + "\'";
            return wrapQuery(queryCommand);
        }
    };
};
