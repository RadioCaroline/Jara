#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>

#include "db_handler/db_connection.h"

using jara_lib::DbType;

class Settings {
public:
    explicit Settings(
        const QString &fileName = "settings.json")
        : _fileName(fileName) {}

    QString getConnectionString(DbType type) {
        QFile file;
        file.setFileName(_fileName);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QString value = file.readAll();
        file.close();
        QJsonDocument document = QJsonDocument::fromJson(value.toUtf8());
        QJsonObject setting = document.object();
        QJsonValue val = setting.value(QString("ConnectionStrings"));
        QJsonObject item = val.toObject();

        switch (type) {
            case DbType::MYSQL:
                return item["MysqlDb"].toString();
            case DbType::ODBC:
                return item["OdbcDb"].toString();
            case DbType::POSTGRES:
                return item["PgsqlDb"].toString();
            case DbType::SQLITE:
            default:
                return "";
        }
    }

    void setConnectionString(DbType type,
        const QString &connectionString) {
        QJsonObject dbObj;

        switch (type) {
            case DbType::MYSQL:
                dbObj.insert("MysqlDb", connectionString);
                break;
            case DbType::ODBC:
                dbObj.insert("OdbcDb", connectionString);
                break;
            case DbType::POSTGRES:
                dbObj.insert("PgsqlDb", connectionString);
                break;
            case DbType::SQLITE:
            default:
                break;
        }
        QJsonObject strObj;
        strObj.insert("ConnectionStrings", dbObj);
        QJsonDocument document;
        document.setObject(strObj);
        QByteArray bytes = document.toJson(
            QJsonDocument::Indented);

        QFile file;
        file.setFileName(_fileName);
        while (file.open(QIODevice::WriteOnly |
                         QIODevice::Text |
                         QIODevice::Truncate)) {
            QTextStream fileStream(&file);
            fileStream.setCodec("utf-8");
            fileStream << bytes;
        }
        file.close();
    }

private:
    QString _fileName;
};


