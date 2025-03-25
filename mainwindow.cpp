#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "usermanagementwidget.h"
#include "filemanagementwidget.h"
#include "projectmanagementwidget.h"
#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUI();
    setupNavigationPanel();
    
    // 默认显示项目管理
    onNavigationClicked(2);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SetCurrentUser(const User& user)
{
    _currentUser = user;
    
    // 设置窗口标题
    QString title = "项目流程文件存储及整理系统";
    if (!_currentUser.userName.isEmpty()) {
        title += QString(" - 当前用户: %1").arg(_currentUser.userName);
        
        switch (_currentUser.role) {
            case UserRole::ADMINISTRATOR:
                title += " (管理员)";
                break;
            case UserRole::PROJECTMANAGER:
                title += " (项目负责人)";
                break;
            case UserRole::NORMALUSER:
                title += " (普通用户)";
                break;
        }
    }
    setWindowTitle(title);
    
    // 更新各模块当前用户
    if (_userManagementWidget)
        _userManagementWidget->setCurrentUser(_currentUser);
    
    if (_fileManagementWidget)
        _fileManagementWidget->setCurrentUser(_currentUser);
    
    if (_projectManagementWidget)
        _projectManagementWidget->setCurrentUser(_currentUser);
    
    // 根据用户角色更新导航面板
    updateNavigationPanel();
}

void MainWindow::setupUI()
{
    // 设置窗口大小和标题
    resize(1200, 800);
    setWindowTitle("项目流程文件存储及整理系统");
    
    // 创建中央部件
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setCentralWidget(centralWidget);
    
    // 创建导航面板
    _navigationPanel = new QWidget(centralWidget);
    _navigationPanel->setFixedWidth(200);
    _navigationPanel->setStyleSheet("background-color: #2C3E50;");
    mainLayout->addWidget(_navigationPanel);
    
    // 创建内容区域
    _contentWidget = new QStackedWidget(centralWidget);
    _contentWidget->setStyleSheet("background-color: #ECF0F1;");
    mainLayout->addWidget(_contentWidget);
    
    // 创建三个主要功能模块
    _userManagementWidget = new UserManagementWidget(_contentWidget);
    _fileManagementWidget = new FileManagementWidget(_contentWidget);
    _projectManagementWidget = new ProjectManagementWidget(_contentWidget);
    
    // 添加到内容区域
    _contentWidget->addWidget(_userManagementWidget);
    _contentWidget->addWidget(_fileManagementWidget);
    _contentWidget->addWidget(_projectManagementWidget);
}

void MainWindow::setupNavigationPanel()
{
    QVBoxLayout* navLayout = new QVBoxLayout(_navigationPanel);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);
    
    // 标题标签
    QLabel* titleLabel = new QLabel("文件管理系统", _navigationPanel);
    titleLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold; padding: 20px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(titleLabel);
    
    // 导航按钮样式
    QString buttonStyle = "QPushButton { "
                          "text-align: left; "
                          "padding: 10px 20px; "
                          "border: none; "
                          "color: white; "
                          "font-size: 14px; "
                          "background-color: transparent; "
                          "} "
                          "QPushButton:hover { "
                          "background-color: #34495E; "
                          "} "
                          "QPushButton:checked { "
                          "background-color: #1ABC9C; "
                          "}";
    
    // 用户管理按钮
    QPushButton* userBtn = new QPushButton("用户管理", _navigationPanel);
    userBtn->setStyleSheet(buttonStyle);
    userBtn->setCheckable(true);
    userBtn->setProperty("index", 0);
    connect(userBtn, &QPushButton::clicked, [this, userBtn]() {
        onNavigationClicked(userBtn->property("index").toInt());
    });
    navLayout->addWidget(userBtn);
    
    // 文件管理按钮
    QPushButton* fileBtn = new QPushButton("文件管理", _navigationPanel);
    fileBtn->setStyleSheet(buttonStyle);
    fileBtn->setCheckable(true);
    fileBtn->setProperty("index", 1);
    connect(fileBtn, &QPushButton::clicked, [this, fileBtn]() {
        onNavigationClicked(fileBtn->property("index").toInt());
    });
    navLayout->addWidget(fileBtn);
    
    // 项目管理按钮
    QPushButton* projectBtn = new QPushButton("项目管理", _navigationPanel);
    projectBtn->setStyleSheet(buttonStyle);
    projectBtn->setCheckable(true);
    projectBtn->setProperty("index", 2);
    connect(projectBtn, &QPushButton::clicked, [this, projectBtn]() {
        onNavigationClicked(projectBtn->property("index").toInt());
    });
    navLayout->addWidget(projectBtn);
    
    navLayout->addStretch();
    
    // 注销按钮
    QPushButton* logoutBtn = new QPushButton("注销", _navigationPanel);
    logoutBtn->setStyleSheet(buttonStyle);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    navLayout->addWidget(logoutBtn);
}

void MainWindow::updateNavigationPanel()
{
    // 根据用户角色调整导航面板
    QList<QPushButton*> buttons = _navigationPanel->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        bool index = btn->property("index").isValid();
        int btnIndex = btn->property("index").toInt();
        
        // 普通用户不能访问用户管理
        if (index && btnIndex == 0 && _currentUser.role == UserRole::NORMALUSER) {
            btn->setVisible(false);
        } else if (index) {
            btn->setVisible(true);
        }
    }
}

void MainWindow::onNavigationClicked(int index)
{
    _contentWidget->setCurrentIndex(index);
    
    // 更新按钮状态
    QList<QPushButton*> buttons = _navigationPanel->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        if (btn->property("index").isValid()) {
            btn->setChecked(btn->property("index").toInt() == index);
        }
    }
}

void MainWindow::onLogout()
{
    if (QMessageBox::question(this, "注销", "确定要注销当前用户吗？",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        hide();
        
        // 显示登录对话框
        LoginDialog loginDialog;
        if (loginDialog.exec() == QDialog::Accepted) {
            // 设置当前用户
            SetCurrentUser(loginDialog.GetLoginUser());
            show();
        } else {
            // 用户取消登录，关闭应用
            close();
        }
    }
}
