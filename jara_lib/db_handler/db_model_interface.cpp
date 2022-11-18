#include "db_model_interface.h"

namespace jara_lib {
    const QHash<QueryClause, QString> _clauses_ {
        { QueryClause::SELECT, "SELECT" },
        { QueryClause::FROM, "FROM" },
        { QueryClause::JOIN, "JOIN" },
        { QueryClause::WHERE, "WHERE" },
        { QueryClause::ORDERBY, "ORDER BY" },
        { QueryClause::DESC, "DESC" },
    };

    const QHash<ColumnOperator, QString> _column_operators_ {
        { ColumnOperator::OR, "OR" },
        { ColumnOperator::AND, "AND" },
        { ColumnOperator::EQUAL, "=" },
        { ColumnOperator::NOTEQUAL, "<>" },
        { ColumnOperator::GREATEROREQ, ">=" },
        { ColumnOperator::GREATER, ">" },
        { ColumnOperator::LESSOREQ, "<=" },
        { ColumnOperator::LESS, "<" },
    };

    TableRegister IEntityModel::_register_tables_;
    ColumnRegister IEntityModel::_register_columns_;
}
