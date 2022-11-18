#pragma once

/*
 * Исходники для виртуальных интерфейсов моделей. Объявлены общий интерфейс
 * для моделей таблиц и колонок, и отдельные интерфейсы, которые наследованы
 * от общего интерфейса, но при этом объявляют некоторые собственные методы.
 * Тем самым образуются связи между таблицами и колонками баз данных.
 * Также определен интерфейс для контекстов.
 */

#include <map>
#include <memory>
#include <QSet>
#include <QPair>
#include <QVariant>
#include <QSqlQuery>
#include <QSharedPointer>

namespace jara_lib {
    /*
     * Для итерации по возвращаемым коллекциям указателей из методов.
     * Без этой функции выйдет warning:
     * Using C++11 range-based for loop correctly in Qt
     */
    template <class T>
    T const as_const(T &&t) { return std::forward<T>(t); }

    /*!
     * Типы СУБД, которые могут использоваться для базы данных:
     * 1. Sqlite, 2. MySQL/MariaDb, 3. PostgreSQL, 4. ODBC (MS SQL Server)
     */
    enum DbType : ushort { SQLITE = 1, MYSQL, POSTGRES, ODBC };
    enum CommandResult : ushort { OK = 0, ENTITY_EXISTS };

    enum ColumnOperator : ushort {
        OR, AND, EQUAL, NOTEQUAL,
        GREATEROREQ, GREATER,
        LESSOREQ, LESS
    };

    /*!
     *  Типы колонок:
     *  целочисленные или вещественные числа, строки
     */
    enum ColumnType : ushort {
        INT,                // целочисленный
        INT_NULL,           // целочисленный (нулевой)
        INT_SERIAL,         // целочисленный (счётчик)
        BIGINT,             // большое целочисленное
        BIGINT_NULL,        // большое целочисленное (нулевой)
        BIGINT_SERIAL,      // большое целочисленное (счётчик)
        STRING,             // текстовое
        STRING_NULL         // текстовое (нулевое)
    };

    /*!
     *  Типы полей в базе данных:
     *  первичный/вторичный ключ, обычное поле
     */
    enum FieldType : ushort {
        PRIMARY_KEY,
        FOREIGN_KEY,
        STRAIGHT_FIELD
    };

    /*!
     * Состояния полей в структуре:
     * UNCHANGED - поле не меняло своего значения,
     * ADDED - поле получило своё значение в процессе исполнения,
     * CHANGED - поле изменило своё значение в процессе исполнения,
     * DELETED - поле сбросило своё значение в процессе исполнения
     */
    enum ValueState : ushort {
        UNCHANGED,
        ADDED,
        CHANGED,
        DELETED
    };

    /*! Команды запросов SQL */
    enum QueryClause : ushort {
        SELECT,
        FROM,
        JOIN,
        WHERE,
        ORDERBY,
        DESC,
    }; 

    /*! Словарь для команд запросов для представления в строковом варианте */
    extern const QHash<QueryClause, QString> _clauses_;
    extern const QHash<ColumnOperator, QString> _column_operators_;

    struct ColumnInfo {
        QString columnName;
        ColumnType columnType;
    };

    // Объявляемые интерфейсы
    //! Общий интерфейс моделей
    class IEntityModel;
    //! Интерфейс для таблиц
    class ITableModel;
    //! Интерфейс колонок
    class IColumnModel;
    //! Интерфейс контекстов
    class IModelContext;
    //! Интерфейс классов выражений
    class IExpressionHandler;

    /* Псевдонимы для различных типов */
    // Указатель на модель таблицы
    using DbTable = ITableModel*;
    // Множество указателей на таблицы
    using DbTables = QSet<DbTable>;
    // Указатель на модель колонки
    using DbColumn = IColumnModel*;
    // Множество указателей на колонки
    using DbColumns = QSet<DbColumn>;
    // Указатель на тип контекста
    using DbContext = IModelContext*;

    // Реестр для хранение имен таблиц и колонок, определяемых через
    // структуры, которые реализуют таблицы и колонки в коде
    using TableRegister = std::map<QString, DbTables>;
    using TableColumn = std::pair<QString*, QString>;
    using ColumnRegister = std::map<TableColumn, DbColumns>;
    // Коллекция из вторичных ключей для связей между таблицами
    using ForeignKeys = QVector<QPair<DbTable, DbColumn>>;
    // Таблица из типа запроса SQL и коллекции получаемых полей
    using ExpressionNodes = QMap<QueryClause, QVector<QString>>;

    //! Общий интерфейс моделей
    class IEntityModel {
    public:
        //! Метод для получения имени модели (IEntityModel)
        virtual QString getModelName() const = 0;
        //! Метод для задания имени модели (IEntityModel)
        virtual void setModelName(const QString&) = 0;

    protected:
        static TableRegister _register_tables_;
        static ColumnRegister _register_columns_;
    };

    //! Интерфейс модели таблиц
    class ITableModel : public IEntityModel {
    public:
        //! Регистрация колонки для связи с таблицей (ITableModel)
        virtual void registerModel(DbColumn) = 0;
        //! Регистрация контекста, к которому принадлежит таблица (ITableModel)
        virtual void registerContext(DbContext context) = 0;
        virtual DbContext getTableContext() const = 0;
        //! Регистрация колонки как первичного ключа для данной таблицы (ITableModel)
        virtual void registerPK(DbColumn) = 0;
        //! Получение колонки - первичного ключа (ITableModel)
        virtual DbColumn getPkColumn() const = 0;
        //! Регистрация вторичного ключа с указанием на таблицу (ITableModel)
        virtual void registerFK(DbTable, DbColumn) = 0;
        //! Получение вторичных ключей данной таблицы (ITableModel)
        virtual ForeignKeys getFkColumns() const = 0;
        //! Получение коллекции колонок (ITableModel)
        virtual DbColumns getTableColumns() const = 0;

    protected:
        //! Флаг состояния значения в таблице (ITableModel)
        ValueState _valueState = ValueState::UNCHANGED;
    };

    //! Интерфейс модели колонки
    class IColumnModel : public IEntityModel {
    public:
        //! Регистрация колонки для связи с таблицей (IColumnModel)
        virtual void registerModel(DbTable) = 0;
        //! Получить указатель на таблицу, к которой привязана колонка (IColumnModel)
        virtual DbTable getTable() const = 0;
        //! Получение типа ключа: первичный или вторичный ключ или обычное поле (IColumnModel)
        virtual FieldType getModelCellType() const = 0;
        //! Получение типа колонки (IColumnModel)
        virtual ColumnType getModelType() const = 0;
        //! Получение значение колонки (IColumnModel)
        virtual QVariant getModelValue() const = 0;
        //! Задать значение колонки (IColumnModel)
        virtual void setModelValue(const QVariant&) = 0;
        //! Изменить значение колонки (IColumnModel)
        virtual void changeModelValue(const QVariant&) = 0;

    protected:
        //! Флаг состояния значения в заданной ячейке (IColumnModel)
        ValueState _valueState = ValueState::UNCHANGED;
    };

    class IModelContext {
    public:
        virtual void registerTable(const DbTable&) = 0;
        virtual QSqlQuery proceedExpression(
            const IExpressionHandler &expression) = 0;
        virtual DbType getDbType() const = 0;
        virtual void dbInit() = 0;
        virtual void migrate() = 0;
    };

    class IExpressionHandler {
    public:
        virtual void setQueryTable(const DbTable&) = 0;
        virtual const ExpressionNodes getExpressionNodes() const = 0;
    };
};
