#pragma once

#include "model_context.h"

namespace jara_lib {

    template <typename T> class PrimaryKey;
    template <> class PrimaryKey<IntColumn> {
    public:
        explicit PrimaryKey<IntColumn>(
            const QString& tableName, DbTable table)
            : _column(IntColumn(
                tableName, table, ColumnType::INT_SERIAL)) {
            _column.setConstraint<TableModel>(
                FieldType::PRIMARY_KEY, nullptr);
        }

        operator QString() const
        { return _column; }

        operator DbColumn()
        { return _column; }

    private:
        IntColumn _column;
    };

    template <> class PrimaryKey<BigIntColumn> {
    public:
        explicit PrimaryKey<BigIntColumn>(
            const QString& tableName, DbTable table)
            : _column(BigIntColumn(
                tableName, table, ColumnType::BIGINT_SERIAL)) {
            _column.setConstraint<TableModel>(
                FieldType::PRIMARY_KEY, nullptr);
        }

        operator QString() const
        { return _column; }

        operator DbColumn()
        { return _column; }

    private:
        BigIntColumn _column;
    };

    template <typename T, typename Table> class ForeignKey;
    template <typename Table> class ForeignKey<IntColumn, Table> {
    public:
        explicit ForeignKey<IntColumn, Table>(const QString& tableName,
                                              DbTable table)
            : _column(IntColumn(tableName, table)) {

            QString tbName =
                abi::__cxa_demangle(typeid(Table).name(),0,0,nullptr);

            _column.setConstraint<TableModel>(FieldType::FOREIGN_KEY,
                static_cast<TableModel*>(new Table(tbName, nullptr)));
        }

        operator QString() const
        { return _column; }

        operator DbColumn()
        { return _column; }

    private:
        IntColumn _column;
    };

    template <typename Table> class ForeignKey<BigIntColumn, Table> {
    public:
        explicit ForeignKey<BigIntColumn, Table>(
            const QString& tableName, DbTable table)
            : _column(BigIntColumn(
                tableName, table)) {

            QString tbName =
                abi::__cxa_demangle(typeid(Table).name(),0,0,nullptr);

            _column.setConstraint<TableModel>(FieldType::FOREIGN_KEY,
                static_cast<TableModel*>(new Table(tbName, nullptr)));
        }

        operator QString() const
        { return _column; }

        operator DbColumn()
        { return _column; }

    private:
        BigIntColumn _column;
    };

    template <typename T> class Nullable;
    template <> class Nullable<IntColumn> {
    public:
        explicit Nullable<IntColumn>(
            const QString& tableName, DbTable table)
            : _column(IntColumn(tableName, table)) {
            _column.setType(ColumnType::INT_NULL);
        }

        operator QString() const
        { return _column; }

    private:
        IntColumn _column;
    };

    template <> class Nullable<StringColumn> {
    public:
        explicit Nullable<StringColumn>(
            const QString &tableName, DbTable table, int size = 0)
            : _column(StringColumn(tableName, table, size)) {
            _column.setType(ColumnType::STRING_NULL);
        }

        operator QString() const
        { return _column; }

    private:
        StringColumn _column;
    };

    using Int = IntColumn;
    using BigInt = BigIntColumn;
    using String = StringColumn;

#define DECLARE_TABLE(Table) Table(const QString &tableName = abi::__cxa_demangle(typeid(Table).name(),0,0,nullptr), DbContext context = nullptr) : TableModel(tableName, context) {}
#define COLUMN(name) name = decltype(name)(#name, this)
};
