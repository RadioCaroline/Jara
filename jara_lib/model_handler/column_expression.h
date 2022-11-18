#pragma once

#include <cxxabi.h>
#include <memory>
#include <QStack>
#include <QDebug>
#include <QSharedPointer>
#include "db_handler/db_model_interface.h"

namespace jara_lib {
    class ExpressionNode;

    class COL {
    public:
        COL(DbColumn = nullptr);

        DbColumn getColumn() const;

    public:
        operator QString() const;
        COL& operator==(const QVariant&);
        COL& operator==(const COL&);
        COL& operator!=(const QVariant&);
        COL& operator!=(const COL&);
        COL operator&&(const COL&) const;
        COL operator||(const COL&) const;

        const QSharedPointer<ExpressionNode>& getExpression() const;

    private:
        DbColumn _column;
        QSharedPointer<ExpressionNode> _expression;
    };

    class ExpressionNode {
    public:
        struct ExpressionVariant {
            ExpressionVariant(const QSharedPointer<COL>& = nullptr,
                              const QVariant& = QVariant(),
                              const QSharedPointer<ExpressionNode>& = nullptr);
            ExpressionVariant(const QVariant&);
            ExpressionVariant(const QSharedPointer<ExpressionNode>&);

            operator QString() const;

            QSharedPointer<COL> _column;
            QVariant _value;
            QSharedPointer<ExpressionNode> _node;
        };

    public:
        ExpressionNode();
        ExpressionNode(const ExpressionVariant&,
                       const ExpressionVariant&,
                       ColumnOperator);

        operator QString() const;

        ExpressionVariant _first;
        ExpressionVariant _second;
        ColumnOperator _operator;
    };

    class ExpressionHandler : public IExpressionHandler {
    protected:
        void setQueryTable(const DbTable&) override;

    private:
        template <class Table>
        QSharedPointer<Table> objectPrepare() {
            QSharedPointer<Table> object =
                QSharedPointer<Table>::create(*(static_cast<Table*>(this)));

            DbColumn primaryKey = object->getPkColumn();

            _expression_nodes_[QueryClause::SELECT].insert(0, COL(primaryKey));
            return object;
        }

        QVector<QString> parseExpression(
            const QSharedPointer<ExpressionNode>&) const;

    public:
        const ExpressionNodes getExpressionNodes() const override
        { return _expression_nodes_; }

        template <class Table>
        Table toObject() {
            QSharedPointer<Table> table = objectPrepare<Table>();
            QSqlQuery records =
                table->getTableContext()->proceedExpression(*this);

            if (records.first()) {
                Table tableObj = Table(table->getModelName(),
                                       table->getTableContext());

                ushort index = 0;
                for (const QString &node :
                     qAsConst(_expression_nodes_[QueryClause::SELECT])) {
                    DbColumn column = tableObj[node];
                    if (column) {
                        column->setModelValue(records.value(index++));
                    }
                }
                return std::move(tableObj);
            }

            return Table();
        }

        template <class Table>
        QVector<Table> toObjectList() {
            QVector<Table> tables;
            QSharedPointer<Table> table = objectPrepare<Table>();
            QSqlQuery records =
                table->getTableContext()->proceedExpression(*this);

            while (records.next()) {
                Table tableObj = Table(table->getModelName(),
                                       table->getTableContext());

                ushort index = 0;
                for (const QString &node :
                     qAsConst(_expression_nodes_[QueryClause::SELECT])) {
                    DbColumn column = tableObj[node];
                    if (column) {
                        column->setModelValue(records.value(index++));
                    }
                }
                tables.append(std::move(tableObj));
            }

            return tables;
        }

        template <typename ...Columns>
        ExpressionHandler& select(Columns ...selectNode) {
            std::array<COL, sizeof...(Columns)> const nodes { selectNode... };
            for (const COL& node : nodes) {
                _expression_nodes_[QueryClause::SELECT]
                    .append(node.operator QString().trimmed());
            }
            return *this;
        }

        template <class Table>
        ExpressionHandler& join(const COL &joinColumn) {
            Table table = Table(
                abi::__cxa_demangle(typeid(table).name(),0,0,nullptr), nullptr);

            DbType dbType = joinColumn.getColumn()->
                getTable()->getTableContext()->getDbType();

            QString tbName = (dbType == DbType::POSTGRES)
                ? "\"" + table.getModelName() + "\""
                : table.getModelName();

            QString joinNode =
                joinColumn.getExpression().data()->operator QString();

            _expression_nodes_[QueryClause::JOIN].append(
                tbName + " ON " + joinNode);

            return *this;
        }

        ExpressionHandler& where(const COL &whereColumn) {
            _expression_nodes_[QueryClause::WHERE] =
                parseExpression(whereColumn.getExpression());

            return *this;
        }

        template <typename ...Columns>
        ExpressionHandler& orderby(Columns ...orderByNode) {
            std::array<COL, sizeof...(Columns)> const nodes { orderByNode... };
            for (const COL& node : nodes) {
                _expression_nodes_[QueryClause::ORDERBY]
                    .append(node.operator QString().trimmed());
            }
            return *this;
        }

        ExpressionHandler& desc() {
            _expression_nodes_[QueryClause::DESC] =
                QVector<QString>();
            return *this;
        }

    private:
        DbTable _table;
        ExpressionNodes _expression_nodes_;
    };
};
