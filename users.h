#ifndef USERS_H
#define USERS_H

#include <QString>
#include <QDateTime>
#include <QSqlDatabase>

enum class UserRole
{
    Administrators = 0,
    ProjectManager,
    NormalUser
};

class Users
{
public:
    Users();

    Users(const QString& userName, const QString& passwd, UserRole role);

    QString GetUserName() const;

    UserRole GetRole() const;

    bool CanCreateProject() const;

    bool CanManageUsers() const;

    bool CanUploadFiles() const;

    bool CanViewFiles() const;

    bool CanEditProject() const;

private:
    QString _userName;

    QString _passwd;

    UserRole _role;

    QDateTime _createdTime;
};

#endif // USERS_H
