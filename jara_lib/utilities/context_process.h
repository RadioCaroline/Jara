#pragma once

#include <QDebug>
#include <QStack>

#include "model_handler/model_context.h"

using jara_lib::ModelContext;

class ContextProcess {
public:
    void migrate(ModelContext *context) {
        if (context) {
            _contexts.push(context);
        }
        qDebug() << "Migrated...";
    }

    void commit() {
        qDebug() << "Commited...";
    }

private:
    QStack<ModelContext*> _contexts;
};
