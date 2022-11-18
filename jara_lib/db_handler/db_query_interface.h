#pragma once

#include <QMap>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "db_model_interface.h"

namespace jara_lib {
    /*! Интерфейс класса для создания запросов в СУБД */
    class IDbCommand {
    public:
        IDbCommand() {
            // Заполняем общие типы для всех СУБД
            ColumnTypes[ColumnType::INT] = "INT NOT NULL";
            ColumnTypes[ColumnType::INT_NULL] = "INT NULL";
            ColumnTypes[ColumnType::BIGINT] = "BIGINT NOT NULL";
            ColumnTypes[ColumnType::BIGINT_NULL] = "BIGINT NULL";
        }
        virtual ~IDbCommand() {
            // Очищаем коллекцию типов
            ColumnTypes.clear();
        }

        /*!
         *  Строка запроса для создания базы данных (IDbCommand);
         *  {dbName} - имя создаваемой базы данных;
         *  {existsCheck} - необязательный параметр проверки существования базы;
         */
        virtual QString createDatabase(const QString &dbName,
                                       bool existsCheck = false) const {

            // Формируем строку запроса на создание базы
            QString queryCommand = "CREATE DATABASE ";
            /*
             * Если поставлен флаг проверки существование базы,
             * добавляем подстроку в запроса
             */
            queryCommand += (existsCheck) ? "IF NOT EXISTS " : "";
            // Записываем название базы
            queryCommand += dbName + "; ";
            return queryCommand;
        }

        /*! Проверка существования базы данных в СУБД (IDbCommand) */
        virtual QString checkDatabase(const QString&) const = 0;

        /*!
         *  Строка запроса для удаления базы данных (IDbCommand);
         *  {dbName} - имя удаляемой базы данных;
         *  {existsCheck} - необязательный параметр проверки существования базы;
         */
        virtual QString dropDatabase(const QString &dbName,
                                     bool existsCheck = false) const {
            // Формируем строку запроса на удаление базы
            QString queryCommand = "DROP DATABASE ";
            /*
             * Если поставлен флаг проверки существование базы,
             * добавляем подстроку в запроса
             */
            queryCommand += (existsCheck) ? "IF EXISTS " : "";
            // Записываем название базы
            queryCommand += dbName + ";";
            return queryCommand;
        }

        /*!
         *  Строка запроса для создания таблицы в базе (IDbCommand);
         *  {table} - указатель на модель создаваемой таблицы;
         *  {dbName} - имя базы данных, в которой создаётся таблица;
         *  {existsCheck} - необязательный параметр проверки существования таблицы;
         */
        virtual QString createTable(const QString &dbName,
                                    const DbTable &table,
                                    bool existsCheck = false) const {

            // Создаём строку для создания таблицы
            QString queryCommand = "CREATE TABLE ";
            // Добавляем проверку существования таблицы, если поставлен флаг
            queryCommand += (existsCheck) ? "IF NOT EXISTS " : "";
            /*
             * Добавляем имя базы, в которой будет создана таблица и через точку
             * имя создаемой таблицы.
             */
            queryCommand += dbName + "." + table->getModelName() + "(";

            // Добавляем колонки
            queryCommand += prepareColumns(table->getTableColumns());

            // Добавляем запрос на создание первичного ключа
            queryCommand += primaryConstraint(table->getPkColumn());

            queryCommand += "); ";

            return queryCommand;
        }

        /*! Проверка существование таблицы в базе данных (IDbCommand) */
        virtual QString checkTable(const QString&, const DbTable&) const = 0;

        /*!
         *  Строка запроса для удаления таблицы в базе (IDbCommand);
         *  {tbName} - имя удаляемой таблицы;
         *  {dbName} - имя базы данных, в которой удаляется таблица;
         *  {existsCheck} - необязательный параметр проверки существования таблицы;
         */
        virtual QString dropTable(const QString &dbName,
                                  const DbTable &table,
                                  bool existsCheck = false) const {

            // Создаём строку для удаления таблицы
            QString queryCommand = "DROP TABLE ";
            // Добавляем проверку существования таблицы, если поставлен флаг
            queryCommand += (existsCheck) ? "IF EXISTS " : "";
            /*
             * Добавляем имя базы, в которой будет создана таблица
             * и через точку имя создаемой таблицы.
             */
            queryCommand += dbName + "." + table->getModelName() + "; ";

            return queryCommand;
        }

        virtual QString prepareColumn(const DbColumn &column,
                                      bool cleared = false) const {

            QString queryCommand = column->getModelName() + " ";
            queryCommand +=
                (!cleared) ? ColumnTypes[column->getModelType()] + " " : "";

            return queryCommand;
        }

        virtual QString prepareColumns(const DbColumns &tableColumns) const {
            QString columns = "";
            // Проходим по коллекции колонок таблицы
            for (const DbColumn &column : as_const(tableColumns)) {
                // и добавляем в запрос указание на создание колонки
                columns +=
                    prepareColumn(column) + ", ";

            }
            return columns;
        }

        /*!
         *  Изменение таблицы: добавление колонки (IDbCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя добавляемой колонки;
         *  {type} - тип добавляемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        virtual QString alterAddColumn(const QString &dbName,
                                       const DbColumn &column,
                                       bool existsCheck = false) const {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE " + dbName + ".";
            // Вносим имя редактируемой таблицы
            queryCommand += column->getTable()->getModelName() + " ADD ";
            // Если флаг проверки поставлен
            if (existsCheck) {
                // добавляем строку на проверку существования колонки
                queryCommand += "IF NOT EXISTS ";
            }
            // Вносим имя и тип колонки
            queryCommand += prepareColumn(column) + "; ";

            return queryCommand;
        }

        /*!
         *  Изменение таблицы: изменение колонки (IDbCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя изменяемой колонки;
         *  {type} - тип изменяемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        virtual QString alterModifyColumn(const QString &dbName,
                                          const DbColumn &column,
                                          bool existsCheck = false) const {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE " + dbName + ".";
            // Вносим имя редактируемой таблицы
            queryCommand += column->getTable()->getModelName() + " MODIFY COLUMN ";
            // Если флаг проверки поставлен
            if (existsCheck) {
                // добавляем строку на проверку существования колонки
                queryCommand += "IF EXISTS ";
            }
            // Вносим имя и тип колонки
            queryCommand += prepareColumn(column) + "; ";

            return queryCommand;
        }

        /*!
         *  Изменение таблицы: удаление колонки (IDbCommand);
         *  {dbName} - имя базы данных, в которой редактируется таблица;
         *  {tbName} - имя редактируемой таблицы;
         *  {clName} - имя удаляемой колонки;
         *  {type} - тип удаляемой колонки;
         *  {existsCheck} - необязательный параметр проверки существования колонки;
         */
        virtual QString alterDropColumn(const QString &dbName,
                                        const DbColumn &column,
                                        bool existsCheck = false) const {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE " + dbName + ".";
            // Вносим имя редактируемой таблицы
            queryCommand += column->getTable()->getModelName() + " DROP COLUMN ";
            // Если флаг проверки поставлен
            if (existsCheck) {
                // добавляем строку на проверку существования колонки
                queryCommand += "IF EXISTS ";
            }
            // Вносим имя колонки
            queryCommand += prepareColumn(column, true) + "; ";

            return queryCommand;
        }

        virtual QString preparePrimaryKey(const DbColumn &column) const {
            QString preparedKey = "PK_" + column->getTable()->getModelName();
            preparedKey += "_" + column->getModelName();
            return preparedKey;
        }

        virtual QString primaryConstraint(const DbColumn &column) const {

            QString queryCommand = "CONSTRAINT ";
            queryCommand += preparePrimaryKey(column);
            queryCommand += " PRIMARY KEY (" + column->getModelName() + ") ";

            return queryCommand;
        }

        virtual QString prepareForeignKey(const DbTable &fTable,
                                          const DbTable &pTable) const {
            QString queryCommand = "FK_";
            queryCommand += fTable->getModelName() + "_";
            queryCommand += pTable->getModelName();
            return queryCommand;
        }

        virtual QString foreignConstraint(const DbColumn &foreign,
                                          const DbColumn &primary) const {

            QString queryCommand =
                prepareForeignKey(foreign->getTable(), primary->getTable());
            queryCommand += " FOREIGN KEY(" + foreign->getModelName();
            queryCommand += ") REFERENCES ";
            queryCommand += primary->getTable()->getModelName();
            queryCommand += "(" + primary->getModelName() + ") ";

            return queryCommand;
        }

        virtual QString checkForeignKey(const QString &dbName,
                                        const DbTable &fTable,
                                        const DbTable &pTable) const = 0;

        virtual QString alterAddConstraint(const QString &dbName,
                                           const DbColumn &foreign,
                                           const DbColumn &primary) const {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE " + dbName + ".";
            // Вносим имя редактируемой таблицы
            queryCommand += foreign->getTable()->getModelName();
            queryCommand += " ADD CONSTRAINT ";
            // Вносим имя и тип колонки
            queryCommand += foreignConstraint(foreign, primary) + "; ";

            return queryCommand;
        }

        virtual QString alterDropConstraint(const QString &dbName,
                                           const DbColumn &foreign,
                                           const DbColumn &primary) const {

            // Создаём строку изменения таблицы, добавляем названия базы
            QString queryCommand = "ALTER TABLE " + dbName + ".";
            // Вносим имя редактируемой таблицы
            queryCommand += foreign->getTable()->getModelName();
            queryCommand += " DROP CONSTRAINT ";
            // Вносим имя и тип колонки
            queryCommand += prepareForeignKey(
                foreign->getTable(), primary->getTable()) + "; ";

            return queryCommand;
        }

        virtual QString getTableColumns(const QString &dbName,
                                        const DbTable &table) const = 0;

        virtual QString getTableColumn(const QString &dbName,
                                       const DbColumn &column) const = 0;

        virtual QString makeExpressionClause(
            QueryClause clause, const QVector<QString> &columns) const {
            QString expressionPart = "";
            if (clause == QueryClause::JOIN) {
                for (const QString &node : as_const(columns)) {
                    expressionPart += _clauses_[clause] + " " + node + " ";
                }
            }
            else if (clause == QueryClause::WHERE) {
                expressionPart += _clauses_[clause] + " ";
                for (const QString &node : as_const(columns)) {
                    expressionPart += node + " ";
                }
            }
            else {
                expressionPart += _clauses_[clause] + " ";
                if (columns.count()) {
                    for (const QString &node : as_const(columns)) {
                        expressionPart += node + ", ";
                    }
                    expressionPart = expressionPart
                        .remove(expressionPart.lastIndexOf(','), 1);
                }
            }
            return expressionPart;
        }

        static QMap<ColumnType, QString> ColumnTypes;
    };

    QMap<ColumnType, QString> IDbCommand::ColumnTypes;
}
