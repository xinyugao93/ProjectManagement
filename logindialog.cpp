#include "logindialog.h"
#include "ui_logindialog.h"
#include <QFormLayout>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("登录");
    ui->lineEdit_2->setPlaceholderText("请输入用户名");
    ui->lineEdit->setPlaceholderText("请输入密码");
    ui->lineEdit->setEchoMode(QLineEdit::Password);
    
    // 断开自动生成的信号连接，防止在验证失败时对话框被自动接受
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    // 只处理"确定"按钮
    if (ui->buttonBox->standardButton(button) != QDialogButtonBox::Yes)
        return;
        
    QString userName = ui->lineEdit_2->text();
    QString password = ui->lineEdit->text();
    if(userName.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "错误", "用户名和密码不能为空");
        return;
    }

    User user;
    QString errMsg;
    if(!_userManagement.ValidateLogin(userName, password, user, errMsg))
    {
        QMessageBox::warning(this, "错误", errMsg);
        ui->lineEdit->clear();
        ui->lineEdit->setFocus();
        // 不调用accept()，所以对话框保持打开状态
    }
    else
    {
        // 只有在验证成功时才接受对话框
        accept();
    }
}

User LoginDialog::GetLoginUser() const
{
    return _userManagement.GetLoginUser();
}

