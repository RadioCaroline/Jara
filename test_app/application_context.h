#pragma once

#include "employee_table.h"

struct ApplicationContext : public ModelContext {
    DECLARE_CONTEXT(ApplicationContext)

    EmployeeTable TABLE(employees);
    DepartmentTable TABLE(departments);
    CompanyTable TABLE(companies);
};
