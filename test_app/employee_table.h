#pragma once

#include "model_handler/column_types.h"
#include "model_handler/table_model.h"

using namespace jara_lib;

struct EmployeeTable;
struct DepartmentTable;
struct CompanyTable;

struct EmployeeTable : public TableModel {
    DECLARE_TABLE(EmployeeTable)  

    PrimaryKey<Int> COLUMN(Id);
    ForeignKey<Int, DepartmentTable> COLUMN(DepartmentId);
    String COLUMN(FirstName);
    String COLUMN(LastName);
    String COLUMN(Patronymic);
};

struct DepartmentTable : public TableModel {
    DECLARE_TABLE(DepartmentTable)

    PrimaryKey<Int> COLUMN(Id);
    ForeignKey<Int, CompanyTable> COLUMN(CompanyId);
    String COLUMN(Name);
};

struct CompanyTable : public TableModel {
    DECLARE_TABLE(CompanyTable)

    PrimaryKey<Int> COLUMN(Id);
    String COLUMN(Name);
};
