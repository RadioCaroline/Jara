#pragma once

#include "db_query_interface.h"

namespace jara_lib {
    struct MysqlCommand : public IDbCommand {
        MysqlCommand() {
            ColumnTypes[ColumnType::INT_SERIAL] = "INT NOT NULL AUTO_INCREMENT";
            ColumnTypes[ColumnType::BIGINT_SERIAL] = "BIGINT NOT NULL AUTO_INCREMENT";
            ColumnTypes[ColumnType::STRING] = "TEXT NOT NULL";
            ColumnTypes[ColumnType::STRING_NULL] = "TEXT NULL";
        }

        QString checkDatabase(const QString &dbName) const override {
            QString queryCommand = "SELECT * FROM INFORMATION_SCHEMA.SCHEMATA";
            queryCommand += " WHERE SCHEMA_NAME";
            queryCommand += " = \'" + dbName + "\'";
            return queryCommand;
        }

        QString checkTable(const QString &dbName,
                           const DbTable &table) const override {
            QString queryCommand = "SELECT COUNT(*)";
            queryCommand += " FROM INFORMATION_SCHEMA.TABLES";
            queryCommand += " WHERE TABLE_SCHEMA = DATABASE()";
            queryCommand += " AND TABLE_NAME = \'";
            queryCommand += table->getModelName() + "\' AND ";
            queryCommand += "TABLE_SCHEMA = \'" + dbName + "\'; ";
            return queryCommand;
        }

        QString checkForeignKey(const QString &dbName,
                                const DbTable &fTable,
                                const DbTable &pTable) const override {
            QString foreignKey = prepareForeignKey(fTable, pTable);
            QString queryCommand = "SELECT COUNT(*) ";
            queryCommand += "FROM INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS ";
            queryCommand += "WHERE CONSTRAINT_SCHEMA = \'" + dbName + "\'";
            queryCommand += "AND CONSTRAINT_NAME = \'" + foreignKey + "\'";
            return queryCommand;
        }

        QString getTableColumns(const QString &dbName,
                                const DbTable &table) const override {
            QString queryCommand = "SELECT COLUMN_NAME, DATA_TYPE, ";
            queryCommand += "IS_NULLABLE, CHARACTER_OCTET_LENGTH, EXTRA ";
            queryCommand += "FROM INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += table->getModelName() + "\' AND ";
            queryCommand += "TABLE_SCHEMA = \'" + dbName + "\'; ";
            return queryCommand;
        }

        QString getTableColumn(const QString &dbName,
                               const DbColumn &column) const override {
            QString queryCommand = "SELECT COLUMN_NAME, DATA_TYPE, ";
            queryCommand += "IS_NULLABLE, EXTRA ";
            queryCommand += "FROM INFORMATION_SCHEMA.COLUMNS ";
            queryCommand += "WHERE TABLE_NAME = \'";
            queryCommand += column->getTable()->getModelName() + "\' AND ";
            queryCommand += "TABLE_SCHEMA = \'" + dbName + "\' AND ";
            queryCommand += "COLUMN_NAME = \'" + column->getModelName() + "\'; ";
            return queryCommand;
        }
    };
};
