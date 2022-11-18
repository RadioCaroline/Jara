#include <QCoreApplication>

#include "application_context.h"
#include "utilities/settings.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DbType type = DbType::POSTGRES;
    QString connectionString =
        "Server=storage;Port=5051;uid=postgres;pwd=Qwerty123;database=measuringtoolscontroldb";
//    QString connectionString =
//        "Server=.\\SQLEXPRESS03;uid=sa;pwd=sqlMS1406;database=measuringtoolscontroldb";

    Settings settings;
    settings.setConnectionString(type, connectionString);

    try {
        ApplicationContext context(
            DbConnection(settings.getConnectionString(type), type));
        qDebug() << "That's it! The application database is "
                 << "created and matches to columns.";

        QVector<EmployeeTable> employees =
                    context.employees
                        .select(COL(context.employees.FirstName),
                                COL(context.employees.LastName),
                                COL(context.employees.Patronymic),
                                COL(context.companies.Name),
                                COL(context.departments.Name))
                        .join<DepartmentTable>(COL(context.employees.DepartmentId) ==
                                               COL(context.departments.Id))
                        .join<CompanyTable>(COL(context.departments.CompanyId) ==
                                            COL(context.companies.Id))
                        .where((COL(context.employees.Id) == 2 ||
                                (COL(context.employees.LastName) == "Lee" &&
                                 COL(context.employees.LastName) == "Low") ||
                                COL(context.employees.Id) == 8 ||
                                COL(context.employees.Id) == 11) &&
                               (COL(context.employees.DepartmentId) == 1 ||
                                COL(context.employees.DepartmentId) == 3))
                        .orderby(COL(context.employees.Id),
                                 COL(context.departments.Id))
                        .desc()
                        .toObjectList<EmployeeTable>();

        /*context.employees.insert(employee);
        context.commit();
        context.employees.drop(employee);
        context.commit();*/
        qDebug() << "Context is created";
    }  catch (const QString &message) {
        qDebug() << message;
    }

    qDebug() << "The final point is reached";

    return a.exec();
}
