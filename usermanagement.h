#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include <QObject>
#include "DBModels.h"

class UserManagement : public QObject
{
    Q_OBJECT
public:
    explicit UserManagement(QObject *parent = nullptr);

    bool ValidateLogin(const QString& userName, const QString& password, User& user, QString& errMsg);

    User GetLoginUser() const;

private:
    User _dbUser;
};

#endif // USERMANAGEMENT_H
