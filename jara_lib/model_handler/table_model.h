#pragma once

#include <map>
#include <cxxabi.h>
#include "column_expression.h"
#include "db_handler/db_model_interface.h"

namespace jara_lib {
    using TableColumns = std::map<QString, DbColumn>;

    class TableModel :
        public ITableModel, public ExpressionHandler {
    public:
        explicit TableModel(const QString &tableName)
        { setTableName(tableName); }

        explicit TableModel(const QString &tableName,
                            DbContext context)
            : TableModel(tableName) {
            if (context) {
                context->registerTable(this);
                setQueryTable(static_cast<DbTable>(this));
            }
        }

        virtual ~TableModel() {
            if (_tableName) {
                _register_tables_[*_tableName].remove(this);
                if (_register_tables_[*_tableName].count() == 0) {
                    _register_tables_.erase(*_tableName);
                }
            }
        }

        void setTableName(const QString &tableName) {
            QString tbName = tableName;
            if (tbName.contains("Table")) {
                tbName.replace("Table", "");
            }

            if (_tableName) {
                *_tableName = tbName;
            }
            else {
                _register_tables_[tbName].insert(this);
                TableRegister::iterator reg = _register_tables_.find(tbName);
                _tableName = const_cast<QString*>(&reg->first);
            }
        }

        QString getTableName() const
        { return (_tableName) ? *_tableName : ""; }

        QString getModelName() const override
        { return getTableName(); }

        void setModelName(const QString &tableName) override
        { setTableName(tableName); }

        void registerModel(DbColumn column) override
        { _tableColumns.insert(column); }

        DbColumns getTableColumns() const override
        { return _tableColumns; }

        void registerContext(DbContext context) override {
            if (!_tableContext) {
                _tableContext = context;
            }
        }

        DbContext getTableContext() const override
        { return _tableContext; }

        DbColumn operator[](QString columnName) {
            columnName = columnName.replace("\"", "");
            const QString &clName = (columnName.contains(*_tableName + "."))
                ? columnName.mid(columnName.indexOf('.') + 1)
                : columnName;
            for (DbColumn column : as_const(getTableColumns())) {
                if (column->getModelName() == clName) {
                    return column;
                }
            }
            return nullptr;
        }

    public:
        void registerPK(DbColumn column) override
        { _pkColumn = column ; }

        DbColumn getPkColumn() const override
        { return _pkColumn; }

        void registerFK(DbTable table, DbColumn column) override {
            if (table && column) {
                QPair<DbTable, DbColumn> pfBinded =
                    QPair<DbTable, DbColumn>(table, column);

                if (!_fkColumns.contains(pfBinded)) {
                    _fkColumns.append(pfBinded);
                }
            }
        }

        ForeignKeys getFkColumns() const override
        { return _fkColumns; }

    private:
        QString *_tableName = nullptr;
        DbColumn _pkColumn;
        ForeignKeys _fkColumns;
        DbColumns _tableColumns;
        DbContext _tableContext = nullptr;
    };

#define DECLARE_CONTEXT(Context) Context(const DbConnection& connection) : ModelContext(connection) { dbInit(); }
#define TABLE(nm) nm = decltype(nm)(abi::__cxa_demangle(typeid(nm).name(),0,0,nullptr), this)
};
