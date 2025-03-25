#include "usermanagement.h"
#include "Databasemanagement.h"

UserManagement::UserManagement(QObject* parent) : QObject(parent)
{

}

bool UserManagement::ValidateLogin(const QString& userName, const QString& password, User& user, QString& errMsg)
{
    _dbUser = DataBaseManagement::Instance()->GetUserbyUserName(userName);
    if(_dbUser.id < 0)
    {
        errMsg = "输入的用户名不正确";
        return false;
    }

    if(_dbUser.password == password)
    {
        user = _dbUser;
        return true;
    }

    errMsg = "输入的密码不正确";
    return false;
}

User UserManagement::GetLoginUser() const
{
    return _dbUser;
}
