#include "usermanagementwidget.h"
#include "Databasemanagement.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <QVariant>
#include <functional>

UserManagementWidget::UserManagementWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

UserManagementWidget::~UserManagementWidget()
{
}

void UserManagementWidget::setCurrentUser(const User &user)
{
    _currentUser = user;
    updateUIBasedOnRole();
    loadUserData(); // 重新加载用户数据
}

void UserManagementWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 创建顶部工具栏
    QHBoxLayout* toolLayout = new QHBoxLayout();
    
    // 创建角色筛选下拉框
    QString labelName = "筛选角色:";
    QLabel* filterLabel = new QLabel(labelName, this);
    _roleFilter = new QComboBox(this);
    _roleFilter->addItem("所有用户", QVariant(-1));
    _roleFilter->addItem("管理员", QVariant(static_cast<int>(UserRole::ADMINISTRATOR)));
    _roleFilter->addItem("项目负责人", QVariant(static_cast<int>(UserRole::PROJECTMANAGER)));
    _roleFilter->addItem("普通用户", QVariant(static_cast<int>(UserRole::NORMALUSER)));
    connect(_roleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UserManagementWidget::onRoleChanged);
    
    toolLayout->addWidget(filterLabel);
    toolLayout->addWidget(_roleFilter);
    
    // 添加搜索框
    _searchBox = new QLineEdit(this);
    _searchBox->setPlaceholderText("搜索用户...");
    connect(_searchBox, &QLineEdit::returnPressed,
            this, &UserManagementWidget::onRefreshTable);
    toolLayout->addWidget(_searchBox);
    
    toolLayout->addStretch();
    
    // 创建操作按钮
    _addButton = new QPushButton("添加用户", this);
    _editButton = new QPushButton("编辑用户", this);
    _deleteButton = new QPushButton("删除用户", this);
    _refreshButton = new QPushButton("刷新", this);
    
    connect(_addButton, &QPushButton::clicked,
            this, &UserManagementWidget::onAddUser);
    connect(_editButton, &QPushButton::clicked,
            this, &UserManagementWidget::onEditUser);
    connect(_deleteButton, &QPushButton::clicked,
            this, &UserManagementWidget::onDeleteUser);
    connect(_refreshButton, &QPushButton::clicked,
            this, &UserManagementWidget::onRefreshTable);
    
    toolLayout->addWidget(_addButton);
    toolLayout->addWidget(_editButton);
    toolLayout->addWidget(_deleteButton);
    toolLayout->addWidget(_refreshButton);
    
    mainLayout->addLayout(toolLayout);
    
    // 创建用户表格
    _usersTable = new QTableWidget(this);
    _usersTable->setColumnCount(5);
    _usersTable->setHorizontalHeaderLabels({"ID", "用户名", "密码", "角色", "创建时间"});
    _usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _usersTable->setAlternatingRowColors(true);
    
    mainLayout->addWidget(_usersTable);
    
    // 初始加载数据
    loadUserData();
}

void UserManagementWidget::loadUserData()
{
    _usersTable->setRowCount(0);
    
    // 获取所有用户
    QVector<User> users = DataBaseManagement::Instance()->GetAllUsers();
    
    // 应用筛选
    int roleFilter = _roleFilter->currentData().toInt();
    QString searchText = _searchBox->text().trimmed().toLower();
    
    for(const User& user : users)
    {
        // 应用角色筛选
        if(roleFilter != -1 && static_cast<int>(user.role) != roleFilter)
            continue;
        
        // 应用搜索筛选
        if(!searchText.isEmpty() && !user.userName.toLower().contains(searchText))
            continue;
        
        int row = _usersTable->rowCount();
        _usersTable->insertRow(row);
        
        _usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        _usersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
        
        // 密码显示为*号
        QString maskedPassword;
        for(int i = 0; i < user.password.length(); i++)
            maskedPassword += "*";
        _usersTable->setItem(row, 2, new QTableWidgetItem(maskedPassword));
        
        // 角色名称
        QString roleName;
        switch(user.role)
        {
            case UserRole::ADMINISTRATOR:
                roleName = "管理员";
                break;
            case UserRole::PROJECTMANAGER:
                roleName = "项目负责人";
                break;
            case UserRole::NORMALUSER:
                roleName = "普通用户";
                break;
        }
        _usersTable->setItem(row, 3, new QTableWidgetItem(roleName));
        
        // 创建时间
        _usersTable->setItem(row, 4, new QTableWidgetItem(user.createTime.toString("yyyy-MM-dd hh:mm:ss")));
    }
}

void UserManagementWidget::updateUIBasedOnRole()
{
    // 只有管理员可以管理用户
    bool isAdmin = (_currentUser.role == UserRole::ADMINISTRATOR);
    _addButton->setEnabled(isAdmin);
    _editButton->setEnabled(isAdmin);
    _deleteButton->setEnabled(isAdmin);
    
    // 非管理员只能查看
    if(!isAdmin)
    {
        _searchBox->setEnabled(true);
        _refreshButton->setEnabled(true);
        _roleFilter->setEnabled(true);
    }
    else
    {
        _searchBox->setEnabled(true);
        _refreshButton->setEnabled(true);
        _roleFilter->setEnabled(true);
    }
}

void UserManagementWidget::onAddUser()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR)
    {
        QMessageBox::warning(this, "权限不足", "只有管理员可以添加用户！");
        return;
    }
    
    // 获取用户名
    QString username = QInputDialog::getText(this, "添加用户", "请输入用户名:");
    if(username.isEmpty())
        return;
    
    // 检查用户名是否存在
    User existingUser = DataBaseManagement::Instance()->GetUserbyUserName(username);
    if(existingUser.id >= 0)
    {
        QMessageBox::warning(this, "错误", "用户名已存在！");
        return;
    }
    
    // 获取密码
    QString password = QInputDialog::getText(this, "添加用户", "请输入密码:", QLineEdit::Password);
    if(password.isEmpty())
        return;
    
    // 获取角色
    QStringList roles = {"管理员", "项目负责人", "普通用户"};
    bool ok;
    QString role = QInputDialog::getItem(this, "添加用户", "请选择角色:", roles, 2, false, &ok);
    if(!ok)
        return;
    
    // 创建新用户
    User newUser;
    newUser.userName = username;
    newUser.password = password;
    
    if(role == "管理员")
        newUser.role = UserRole::ADMINISTRATOR;
    else if(role == "项目负责人")
        newUser.role = UserRole::PROJECTMANAGER;
    else
        newUser.role = UserRole::NORMALUSER;
    
    // 添加用户
    if(DataBaseManagement::Instance()->AddUser(newUser))
    {
        QMessageBox::information(this, "成功", "用户添加成功！");
        loadUserData(); // 刷新表格
    }
    else
    {
        QMessageBox::warning(this, "错误", "添加用户失败！");
    }
}

void UserManagementWidget::onEditUser()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR)
    {
        QMessageBox::warning(this, "权限不足", "只有管理员可以编辑用户！");
        return;
    }
    
    // 获取选中行
    int row = _usersTable->currentRow();
    if(row < 0)
    {
        QMessageBox::warning(this, "提示", "请先选择要编辑的用户！");
        return;
    }
    
    // 获取用户ID
    int userId = _usersTable->item(row, 0)->text().toInt();
    QString currentUsername = _usersTable->item(row, 1)->text();
    
    // 获取用户信息
    User user = DataBaseManagement::Instance()->GetUserbyUserName(currentUsername);
    if(user.id < 0)
    {
        QMessageBox::warning(this, "错误", "获取用户信息失败！");
        return;
    }
    
    // 不能编辑自己的角色
    if(user.id == _currentUser.id)
    {
        QMessageBox::warning(this, "提示", "不能修改当前登录用户的信息！");
        return;
    }
    
    // 获取新密码（可选）
    QString newPassword = QInputDialog::getText(this, "编辑用户", "请输入新密码（留空保持不变）:", QLineEdit::Password);
    
    // 获取新角色
    QStringList roles = {"管理员", "项目负责人", "普通用户"};
    int currentRole = static_cast<int>(user.role);
    bool ok;
    QString role = QInputDialog::getItem(this, "编辑用户", "请选择角色:", roles, currentRole, false, &ok);
    if(!ok)
        return;
    
    // 更新用户信息
    if(!newPassword.isEmpty())
        user.password = newPassword;
    
    if(role == "管理员")
        user.role = UserRole::ADMINISTRATOR;
    else if(role == "项目负责人")
        user.role = UserRole::PROJECTMANAGER;
    else
        user.role = UserRole::NORMALUSER;
    
    // 保存更改
    if(DataBaseManagement::Instance()->UpdateUser(user))
    {
        QMessageBox::information(this, "成功", "用户信息更新成功！");
        loadUserData(); // 刷新表格
    }
    else
    {
        QMessageBox::warning(this, "错误", "更新用户信息失败！");
    }
}

void UserManagementWidget::onDeleteUser()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR)
    {
        QMessageBox::warning(this, "权限不足", "只有管理员可以删除用户！");
        return;
    }
    
    // 获取选中行
    int row = _usersTable->currentRow();
    if(row < 0)
    {
        QMessageBox::warning(this, "提示", "请先选择要删除的用户！");
        return;
    }
    
    // 获取用户ID
    int userId = _usersTable->item(row, 0)->text().toInt();
    QString username = _usersTable->item(row, 1)->text();
    
    // 不能删除当前登录用户
    if(username == _currentUser.userName)
    {
        QMessageBox::warning(this, "提示", "不能删除当前登录用户！");
        return;
    }
    
    // 确认删除
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", 
                            QString("确定要删除用户'%1'吗？").arg(username),
                            QMessageBox::Yes | QMessageBox::No);
    if(reply != QMessageBox::Yes)
    {
        return;
    }
    
    // 执行删除
    if(DataBaseManagement::Instance()->DeleteUser(userId))
    {
        QMessageBox::information(this, "成功", "用户删除成功！");
        loadUserData(); // 刷新表格
    }
    else
    {
        QMessageBox::warning(this, "错误", "删除用户失败！可能是系统管理员或该用户有关联的项目。");
    }
}

void UserManagementWidget::onRefreshTable()
{
    loadUserData();
}

void UserManagementWidget::onRoleChanged(int index)
{
    loadUserData();
} 
