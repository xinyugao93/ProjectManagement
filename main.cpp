#include "mainwindow.h"
#include "Databasemanagement.h"
#include "logindialog.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(!DataBaseManagement::Instance()->Initialize())
    {
        QMessageBox::critical(nullptr, "错误", "初始化数据库失败！");
        return -1;
    }

    LoginDialog loginDialog;
    if(loginDialog.exec() != QDialog::Accepted)
    {
        return 0;
    }

    MainWindow w;
    w.SetCurrentUser(loginDialog.GetLoginUser());
    w.show();
    return a.exec();
}
