#pragma once

#include "table_model.h"

namespace jara_lib {
    class ColumnModel : public IColumnModel {
    public:
        explicit ColumnModel(const QString &columnName,
                             DbTable table, ColumnType type)
            : _fieldType(FieldType::STRAIGHT_FIELD), _commandType(type) {
            if (table) {
                registerColumn(table);
            }
            setColumnName(columnName);
        }

        ~ColumnModel() {
            // Если указатель на имя колонки не пуст
            if (_columnName) {
                // Находим в реестре таблиц запись с данным именем
                TableModel *t = static_cast<TableModel*>(_columnTable);
                t->getTableName();
                TableRegister::iterator tb =
                    _register_tables_.find(_columnTable->getModelName());
                // Получаем указатель на имя таблицы
                QString *tableName = const_cast<QString*>(&tb->first);
                // Создаем пару из имени колонки и имени таблицы
                TableColumn tc = std::make_pair(tableName, *_columnName);
                // Удаляем такую запись из реестра колонок
                _register_columns_[tc].remove(this);
                // Если коллекция подобных колонок пуста,
                // удаляем полностью запись таблицы
                if (_register_columns_[tc].count() == 0) {
                    _register_columns_.erase(tc);
                }
            }
        }

        void setColumnName(const QString &columnName) {
            if (_columnName) {
                *_columnName = columnName;
            }
            else {
                TableRegister::iterator tb =
                    _register_tables_.find(_columnTable->getModelName());
                QString *tableName = const_cast<QString*>(&tb->first);
                TableColumn tc = std::make_pair(tableName, columnName);

                _register_columns_[tc].insert(this);
                ColumnRegister::iterator reg = _register_columns_.find(tc);
                _columnName = const_cast<QString*>(&reg->first.second);
            }
        }

        void registerColumn(DbTable table) {
            _columnTable = table;
            _columnTable->registerModel(this);
        }

        void registerModel(DbTable table) override
        { registerColumn(table); }

        DbTable getTable() const override
        { return _columnTable; }

        QString getColumnName() const
        { return (_columnName) ? *_columnName : ""; }

        QString getModelName() const override
        { return getColumnName(); }

        ColumnType getModelType() const override
        { return _commandType; }

        FieldType getModelCellType() const override
        { return _fieldType; }

        void setModelName(const QString &columnName) override
        { setColumnName(columnName); }

        void setType(ColumnType type)
        { _commandType = type; }

        void setValueState(ValueState state)
        { _valueState = state; }

        virtual void setColumnValue(const QVariant &value) = 0;

        void setModelValue(const QVariant &value) override {
            setColumnValue(value);
            setValueState(ValueState::ADDED);
        }

        void changeModelValue(const QVariant &value) override {
            setColumnValue(value);
            setValueState(ValueState::CHANGED);
        }

        template <typename Table>
        void setConstraint(FieldType fieldType, DbTable table) {
            _fieldType = fieldType;
            if (_fieldType == FieldType::PRIMARY_KEY) {
                this->_columnTable->registerPK(this);
            }
            else if (_fieldType == FieldType::FOREIGN_KEY) {
                this->_columnTable->registerFK(table, this);
            }
        }

    public:
        operator QString() const
        { return _columnTable->getModelName() + "." +
                 this->getModelName(); }

        operator DbColumn()
        { return static_cast<DbColumn>(this); }

    protected:
        FieldType _fieldType;
        ColumnType _commandType;

    private:
        QString *_columnName = nullptr;
        DbTable _columnTable;
    };

    class IntColumn : public ColumnModel {
    public:
        explicit IntColumn(const QString &tableName, DbTable table,
                           ColumnType type = ColumnType::INT)
            : ColumnModel(tableName, table, type) {}

        IntColumn(const QString &tableName)
            : IntColumn(tableName, nullptr, ColumnType::INT) {}

        QVariant getModelValue() const override
        { return _value; }

        operator int() const
        { return _value; }

        void setColumnValue(const QVariant &value) override
        { _value = value.toInt(); }

        bool operator==(int value) const
        { return _value == value; }

        bool operator==(const IntColumn &column) const
        { return _value == column._value; }

        IntColumn& operator=(int value) {
            changeModelValue(value);
            return *this;
        }

    private:
        int _value;
    };

    class BigIntColumn : public ColumnModel {
    public:
        explicit BigIntColumn(const QString &tableName, DbTable table,
                           ColumnType type = ColumnType::BIGINT)
            : ColumnModel(tableName, table, type) {}

        BigIntColumn(const QString &tableName)
            : BigIntColumn(tableName, nullptr, ColumnType::BIGINT) {}

        QVariant getModelValue() const override
        { return _value; }

        void setColumnValue(const QVariant &value) override
        { _value = value.toLongLong(); }

        BigIntColumn& operator=(long long value) {
            changeModelValue(value);
            return *this;
        }

    private:
        long long _value;
    };

    class StringColumn : public ColumnModel {
    public:
        explicit StringColumn(const QString &tableName,
                              DbTable table, int size = 0)
            : ColumnModel(tableName, table, ColumnType::STRING_NULL),
              _columnSize(QString("(") + ((size) ? QString::number(size) : "MAX") + ")") {}

        StringColumn(const QString &tableName)
            : StringColumn(tableName, nullptr, ColumnType::STRING_NULL) {}

        QVariant getModelValue() const override
        { return _value; }

        void setColumnValue(const QVariant &value) override
        { _value = value.toString(); }

        StringColumn& operator=(const QString &value) {
            changeModelValue(value);
            return *this;
        }

    private:
        const QString _columnSize;
        QString _value;
    };
};
