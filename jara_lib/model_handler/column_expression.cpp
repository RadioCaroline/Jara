#include "column_expression.h"

namespace jara_lib {
    COL::COL(DbColumn columnNode) :
        _column(columnNode) {}

    DbColumn COL::getColumn() const
    { return _column; }

    COL::operator QString() const {
        if (_column->getTable()->
                getTableContext()->getDbType() == DbType::POSTGRES) {
            return "\"" + _column->getTable()->getModelName()
                    + "\".\"" + _column->getModelName() + "\"";
        }
        return _column->getTable()->getModelName() + "." +
               _column->getModelName();
    }

    COL& COL::operator==(const QVariant &value) {
        QSharedPointer<COL> lhs_column =
            QSharedPointer<COL>::create(*this);
        _expression = QSharedPointer<ExpressionNode>::create(
            lhs_column, value, ColumnOperator::EQUAL);
        return *this;
    }

    COL& COL::operator==(const COL &column) {
        QSharedPointer<COL> lhs_column =
            QSharedPointer<COL>::create(*this);
        QSharedPointer<COL> rhs_column =
            QSharedPointer<COL>::create(column);
        _expression = QSharedPointer<ExpressionNode>::create(
            lhs_column, rhs_column, ColumnOperator::EQUAL);
        return *this;
    }

    COL& COL::operator!=(const QVariant &value) {
        QSharedPointer<COL> column =
            QSharedPointer<COL>::create(*this);
        _expression = QSharedPointer<ExpressionNode>::create(
            column, value, ColumnOperator::NOTEQUAL);
        return *this;
    }

    COL& COL::operator!=(const COL &column) {
        QSharedPointer<COL> lhs_column =
            QSharedPointer<COL>::create(*this);
        QSharedPointer<COL> rhs_column =
            QSharedPointer<COL>::create(column);
        _expression = QSharedPointer<ExpressionNode>::create(
            lhs_column, rhs_column, ColumnOperator::NOTEQUAL);
        return *this;
    }

    COL COL::operator&&(const COL &column) const {
        COL concatenated_expression;

        concatenated_expression._expression =
            QSharedPointer<ExpressionNode>::create(
                this->_expression,
                column._expression,
                ColumnOperator::AND);

        return concatenated_expression;
    }

    COL COL::operator||(const COL &column) const {
        COL concatenated_expression;

        concatenated_expression._expression =
            QSharedPointer<ExpressionNode>::create(
                this->_expression,
                column._expression,
                ColumnOperator::OR);

        return concatenated_expression;
    }

    const QSharedPointer<ExpressionNode>& COL::getExpression() const
    { return _expression; }

    ExpressionNode::ExpressionVariant::ExpressionVariant(
        const QSharedPointer<COL> &column,
        const QVariant& value,
        const QSharedPointer<ExpressionNode> &node)
        : _column(column), _value(value), _node(node) {}

    ExpressionNode::ExpressionVariant::ExpressionVariant(const QVariant& value)
        : ExpressionVariant(nullptr, value) {}

    ExpressionNode::ExpressionVariant::ExpressionVariant(
        const QSharedPointer<ExpressionNode> &node)
        : ExpressionVariant(nullptr, QVariant(), node) {}

    ExpressionNode::ExpressionVariant::operator QString() const {
        return (_column)
            ? _column.data()->operator QString()
            : (_value.userType() == QMetaType::QString)
                ? "\'" + _value.toString() + "\'"
                : _value.toString();
    }

    ExpressionNode::ExpressionNode()
        : _second(ExpressionVariant()) {}

    ExpressionNode::ExpressionNode(
        const ExpressionVariant &first,
        const ExpressionVariant &second,
        ColumnOperator op)
        : _first(first), _second(second), _operator(op) {}


    ExpressionNode::operator QString() const {
        return _first + " " +
             _column_operators_[_operator] + " " +
             _second;
    }

    void ExpressionHandler::setQueryTable(const DbTable &table) {
        _table = table;

        QString tableName = (table->getTableContext()->
            getDbType() == DbType::POSTGRES)
                ? "\"" + table->getModelName() + "\""
                : table->getModelName();

        if (!_expression_nodes_[QueryClause::FROM].contains(tableName))
        { _expression_nodes_[QueryClause::FROM].append(tableName); }
    }

    QVector<QString> ExpressionHandler::parseExpression(
            const QSharedPointer<ExpressionNode> &node) const {
        QVector<QString> expressions;

        if (node.data()->_first._node &&
            node.data()->_second._node) {
            expressions.append("(");
            expressions.append(
                parseExpression(node.data()->_first._node));
            expressions.append(_column_operators_[node.data()->_operator]);
            expressions.append(
                parseExpression(node.data()->_second._node));
            expressions.append(")");
        }
        else if (node.data()->_first._node &&
                 !node.data()->_second._node) {
            expressions.append(
                parseExpression(node.data()->_first._node));
        }
        else if (!node.data()->_first._node &&
                 node.data()->_second._node) {
            expressions.append(_column_operators_[node.data()->_operator]);
            expressions.append(
                parseExpression(node.data()->_second._node));
        }
        else {
            expressions.append(*node);
        }

        return expressions;
    }
};
