#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "DBModels.h"
#include "usermanagement.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    User GetLoginUser() const;

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::LoginDialog *ui;

    UserManagement _userManagement;
};

#endif // LOGINDIALOG_H
