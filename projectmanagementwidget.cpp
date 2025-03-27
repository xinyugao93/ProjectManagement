#include "projectmanagementwidget.h"
#include "Databasemanagement.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QApplication>
#include <QDialog>
#include <QCheckBox>
#include <QFormLayout>
#include <QDateEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QDesktopServices>
#include <QSqlQuery>
#include <QDebug>

ProjectManagementWidget::ProjectManagementWidget(QWidget *parent)
    : QWidget(parent)
    , _currentUser()
    , _currentProject()
{
    setupUI();
    updateUIBasedOnRole();
}

ProjectManagementWidget::~ProjectManagementWidget()
{
}

void ProjectManagementWidget::setCurrentUser(const User &user)
{
    _currentUser = user;
    updateUIBasedOnRole();
    loadProjectData(); // 重新加载项目数据
}

void ProjectManagementWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    _stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(_stackedWidget);
    
    setupProjectListView();
    setupProjectDetailView();
    
    _stackedWidget->setCurrentIndex(0); // 默认显示项目列表视图
}

void ProjectManagementWidget::setupProjectListView()
{
    _projectListView = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_projectListView);
    
    // 搜索和过滤区域
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    _searchBox = new QLineEdit();
    _searchBox->setPlaceholderText("搜索项目...");
    _searchBox->setClearButtonEnabled(true);
    connect(_searchBox, &QLineEdit::returnPressed, this, &ProjectManagementWidget::searchProjects);
    topLayout->addWidget(_searchBox);
    
    _statusFilter = new QComboBox();
    _statusFilter->addItem("全部项目", -1);
    _statusFilter->addItem("进行中", 0);
    _statusFilter->addItem("已完成", 1);
    connect(_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProjectManagementWidget::filterProjects);
    topLayout->addWidget(_statusFilter);
    
    // 按钮区域
    _addProjectButton = new QPushButton("新建项目");
    _editProjectButton = new QPushButton("编辑项目");
    _deleteProjectButton = new QPushButton("删除项目");
    QPushButton* refreshButton = new QPushButton("刷新");
    
    connect(_addProjectButton, &QPushButton::clicked, this, &ProjectManagementWidget::onAddProject);
    connect(_editProjectButton, &QPushButton::clicked, this, &ProjectManagementWidget::onEditProject);
    connect(_deleteProjectButton, &QPushButton::clicked, this, &ProjectManagementWidget::onDeleteProject);
    connect(refreshButton, &QPushButton::clicked, this, &ProjectManagementWidget::onRefreshProjects);
    
    topLayout->addWidget(_addProjectButton);
    topLayout->addWidget(_editProjectButton);
    topLayout->addWidget(_deleteProjectButton);
    topLayout->addWidget(refreshButton);
    
    layout->addLayout(topLayout);
    
    // 项目表格
    _projectsTable = new QTableWidget();
    _projectsTable->setColumnCount(6);
    _projectsTable->setHorizontalHeaderLabels({"ID", "项目名称", "项目经理", "创建时间", "预计完成时间", "状态"});
    _projectsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _projectsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _projectsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _projectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _projectsTable->verticalHeader()->setVisible(false);
    
    connect(_projectsTable, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        int projectId = _projectsTable->item(row, 0)->text().toInt();
        onViewProjectDetail(projectId);
    });
    
    layout->addWidget(_projectsTable);
    
    _stackedWidget->addWidget(_projectListView);
}

void ProjectManagementWidget::setupProjectDetailView()
{
    _projectDetailView = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(_projectDetailView);
    
    // 顶部导航和操作区域
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    _backButton = new QPushButton("返回项目列表");
    connect(_backButton, &QPushButton::clicked, this, &ProjectManagementWidget::onBackToList);
    topLayout->addWidget(_backButton);
    
    topLayout->addStretch();
    
    layout->addLayout(topLayout);
    
    // 项目基本信息区域
    QGroupBox* infoGroupBox = new QGroupBox("项目基本信息");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroupBox);
    
    _projectNameLabel = new QLabel();
    _projectNameLabel->setStyleSheet("font-size: 16pt; font-weight: bold;");
    infoLayout->addWidget(_projectNameLabel);
    
    _projectStatusLabel = new QLabel();
    infoLayout->addWidget(_projectStatusLabel);
    
    _projectDescLabel = new QLabel();
    _projectDescLabel->setWordWrap(true);
    infoLayout->addWidget(_projectDescLabel);
    
    layout->addWidget(infoGroupBox);
    
    // 项目成员区域
    QGroupBox* membersGroupBox = new QGroupBox("项目成员");
    QVBoxLayout* membersLayout = new QVBoxLayout(membersGroupBox);
    
    QHBoxLayout* memberButtonLayout = new QHBoxLayout();
    _assignUsersButton = new QPushButton("分配成员");
    connect(_assignUsersButton, &QPushButton::clicked, this, &ProjectManagementWidget::onAssignUsers);
    memberButtonLayout->addStretch();
    memberButtonLayout->addWidget(_assignUsersButton);
    membersLayout->addLayout(memberButtonLayout);
    
    _projectMembersTable = new QTableWidget();
    _projectMembersTable->setColumnCount(3);
    _projectMembersTable->setHorizontalHeaderLabels({"ID", "用户名", "角色"});
    _projectMembersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _projectMembersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _projectMembersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _projectMembersTable->verticalHeader()->setVisible(false);
    membersLayout->addWidget(_projectMembersTable);
    
    layout->addWidget(membersGroupBox);
    
    // 项目节点区域
    QGroupBox* nodesGroupBox = new QGroupBox("项目节点");
    QVBoxLayout* nodesLayout = new QVBoxLayout(nodesGroupBox);
    
    QHBoxLayout* nodeButtonLayout = new QHBoxLayout();
    _addNodeButton = new QPushButton("添加节点");
    _editNodeButton = new QPushButton("编辑节点");
    _deleteNodeButton = new QPushButton("删除节点");
    
    connect(_addNodeButton, &QPushButton::clicked, this, &ProjectManagementWidget::onAddNode);
    connect(_editNodeButton, &QPushButton::clicked, this, &ProjectManagementWidget::onEditNode);
    connect(_deleteNodeButton, &QPushButton::clicked, this, &ProjectManagementWidget::onDeleteNode);
    
    nodeButtonLayout->addStretch();
    nodeButtonLayout->addWidget(_addNodeButton);
    nodeButtonLayout->addWidget(_editNodeButton);
    nodeButtonLayout->addWidget(_deleteNodeButton);
    nodesLayout->addLayout(nodeButtonLayout);
    
    _projectNodesTable = new QTableWidget();
    _projectNodesTable->setColumnCount(5);
    _projectNodesTable->setHorizontalHeaderLabels({"ID", "节点名称", "创建时间", "预计完成时间", "状态"});
    _projectNodesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _projectNodesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _projectNodesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _projectNodesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _projectNodesTable->verticalHeader()->setVisible(false);
    
    connect(_projectNodesTable, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        if(column == 4) { // 状态列
            int nodeId = _projectNodesTable->item(row, 0)->text().toInt();
            bool isCompleted = _projectNodesTable->item(row, 4)->text() == "已完成";
            onNodeStatusChanged(nodeId, !isCompleted);
        }
    });
    
    nodesLayout->addWidget(_projectNodesTable);
    
    layout->addWidget(nodesGroupBox);
    
    // 添加项目文档区域
    QGroupBox* docsGroupBox = new QGroupBox("项目文档");
    QVBoxLayout* docsLayout = new QVBoxLayout(docsGroupBox);
    
    QHBoxLayout* docButtonLayout = new QHBoxLayout();
    _addDocButton = new QPushButton("添加文档");
    _removeDocButton = new QPushButton("移除文档");
    
    connect(_addDocButton, &QPushButton::clicked, this, &ProjectManagementWidget::onAddDocument);
    connect(_removeDocButton, &QPushButton::clicked, this, &ProjectManagementWidget::onRemoveDocument);
    
    docButtonLayout->addStretch();
    docButtonLayout->addWidget(_addDocButton);
    docButtonLayout->addWidget(_removeDocButton);
    docsLayout->addLayout(docButtonLayout);
    
    _projectDocsTable = new QTableWidget();
    _projectDocsTable->setColumnCount(5);
    _projectDocsTable->setHorizontalHeaderLabels({"ID", "文档名称", "上传者", "关联节点",  "上传时间"});
    _projectDocsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _projectDocsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _projectDocsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    _projectDocsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _projectDocsTable->verticalHeader()->setVisible(false);
    
    connect(_projectDocsTable, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        int fileId = _projectDocsTable->item(row, 0)->text().toInt();
        openDocument(fileId);
    });
    
    docsLayout->addWidget(_projectDocsTable);
    
    layout->addWidget(docsGroupBox);
    
    _stackedWidget->addWidget(_projectDetailView);
}

void ProjectManagementWidget::loadProjectData(const QString& status, const QString& search)
{
    _projectsTable->setRowCount(0);
    
    QVector<Project> projects = DataBaseManagement::Instance()->GetAllProjects();
    
    // 过滤项目状态
    int filterStatus = _statusFilter->currentData().toInt();
    if(filterStatus != -1) {
        QVector<Project> filteredProjects;
        for(const Project& project : projects) {
            if((filterStatus == 1 && project.isCompleted) || 
               (filterStatus == 0 && !project.isCompleted)) {
                filteredProjects.append(project);
            }
        }
        projects = filteredProjects;
    }
    
    // 搜索过滤
    QString searchText = _searchBox->text().trimmed();
    if(!searchText.isEmpty()) {
        QVector<Project> filteredProjects;
        for(const Project& project : projects) {
            if(project.name.contains(searchText, Qt::CaseInsensitive) || 
               project.description.contains(searchText, Qt::CaseInsensitive) ||
               project.managerName.contains(searchText, Qt::CaseInsensitive)) {
                filteredProjects.append(project);
            }
        }
        projects = filteredProjects;
    }
    
    // 填充表格
    for(const Project& project : projects) {
        int row = _projectsTable->rowCount();
        _projectsTable->insertRow(row);
        
        _projectsTable->setItem(row, 0, new QTableWidgetItem(QString::number(project.id)));
        _projectsTable->setItem(row, 1, new QTableWidgetItem(project.name));
        _projectsTable->setItem(row, 2, new QTableWidgetItem(project.managerName));
        _projectsTable->setItem(row, 3, new QTableWidgetItem(project.createTime.toString("yyyy-MM-dd")));
        _projectsTable->setItem(row, 4, new QTableWidgetItem(project.estimatedCompleteTime.toString("yyyy-MM-dd")));
        _projectsTable->setItem(row, 5, new QTableWidgetItem(project.isCompleted ? "已完成" : "进行中"));
        
        // 已完成项目行使用不同的背景色
        if(project.isCompleted) {
            for(int col = 0; col < _projectsTable->columnCount(); col++) {
                QTableWidgetItem* item = _projectsTable->item(row, col);
                item->setBackground(QColor(200, 255, 200)); // 浅绿色
            }
        }
    }
}

void ProjectManagementWidget::loadProjectDetail(int projectId)
{
    _currentProject = DataBaseManagement::Instance()->GetProjectById(projectId);
    
    if(_currentProject.id == -1) {
        QMessageBox::warning(this, "错误", "无法加载项目详情，项目可能已被删除。");
        onBackToList();
        return;
    }
    
    // 更新项目基本信息
    _projectNameLabel->setText(_currentProject.name);
    _projectStatusLabel->setText(QString("状态: %1 | 项目经理: %2 | 创建时间: %3 | 预计完成时间: %4")
                               .arg(_currentProject.isCompleted ? "已完成" : "进行中")
                               .arg(_currentProject.managerName)
                               .arg(_currentProject.createTime.toString("yyyy-MM-dd"))
                               .arg(_currentProject.estimatedCompleteTime.toString("yyyy-MM-dd")));
    _projectDescLabel->setText(_currentProject.description);
    
    // 加载项目成员
    _projectMembersTable->setRowCount(0);
    
    // 使用GetProjectUsers获取项目的所有成员
    QVector<User> projectUsers = DataBaseManagement::Instance()->GetProjectUsers(projectId);
    
    // 添加项目经理（可能不在GetProjectUsers返回的结果中）
    bool hasManager = false;
    
    for(const User& user : projectUsers) {
        int row = _projectMembersTable->rowCount();
        _projectMembersTable->insertRow(row);
        
        _projectMembersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        _projectMembersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
        
        QString roleName;
        switch(user.role) {
            case UserRole::ADMINISTRATOR: roleName = "管理员"; break;
            case UserRole::PROJECTMANAGER: roleName = "项目经理"; break;
            case UserRole::NORMALUSER: roleName = "普通用户"; break;
            default: roleName = "未知"; break;
        }
        
        // 如果是项目经理，特别标注
        if(user.id == _currentProject.managerId) {
            roleName = "项目经理";
            hasManager = true;
        }
        
        _projectMembersTable->setItem(row, 2, new QTableWidgetItem(roleName));
    }
    
    // 如果项目经理不在项目成员列表中，单独添加
    if(!hasManager) {
        // 获取所有用户，找到项目经理
        QVector<User> allUsers = DataBaseManagement::Instance()->GetAllUsers();
        for(const User& user : allUsers) {
            if(user.id == _currentProject.managerId) {
                int row = _projectMembersTable->rowCount();
                _projectMembersTable->insertRow(row);
                
                _projectMembersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
                _projectMembersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
                _projectMembersTable->setItem(row, 2, new QTableWidgetItem("项目经理"));
                break;
            }
        }
    }
    
    // 加载项目节点
    _projectNodesTable->setRowCount(0);
    QVector<ProjectNode> nodes = DataBaseManagement::Instance()->GetProjectNodes(projectId);
    
    for(const ProjectNode& node : nodes) {
        int row = _projectNodesTable->rowCount();
        _projectNodesTable->insertRow(row);
        
        _projectNodesTable->setItem(row, 0, new QTableWidgetItem(QString::number(node.id)));
        _projectNodesTable->setItem(row, 1, new QTableWidgetItem(node.name));
        _projectNodesTable->setItem(row, 2, new QTableWidgetItem(node.creationTime.toString("yyyy-MM-dd")));
        _projectNodesTable->setItem(row, 3, new QTableWidgetItem(node.estimatedCompletionTime.toString("yyyy-MM-dd")));
        _projectNodesTable->setItem(row, 4, new QTableWidgetItem(node.isCompleted ? "已完成" : "进行中"));
        
        // 已完成节点使用不同的背景色
        if(node.isCompleted) {
            for(int col = 0; col < _projectNodesTable->columnCount(); col++) {
                QTableWidgetItem* item = _projectNodesTable->item(row, col);
                item->setBackground(QColor(200, 255, 200)); // 浅绿色
            }
        }
    }
    
    // 加载项目文档
    _projectDocsTable->setRowCount(0);

    // 需要遍历所有项目节点，获取每个节点关联的文件
    QSet<int> loadedFileIds; // 用于避免重复添加相同的文件
    for(const ProjectNode& node : nodes) {
        QVector<FileInfo> nodeFiles = DataBaseManagement::Instance()->GetNodeFiles(node.id);
        
        for(const FileInfo& file : nodeFiles) {
            // 如果该文件已经添加过，则跳过
            if(loadedFileIds.contains(file.id)) {
                continue;
            }
            
            int row = _projectDocsTable->rowCount();
            _projectDocsTable->insertRow(row);
            
            _projectDocsTable->setItem(row, 0, new QTableWidgetItem(QString::number(file.id)));
            _projectDocsTable->setItem(row, 1, new QTableWidgetItem(file.fileName));
            _projectDocsTable->setItem(row, 2, new QTableWidgetItem(file.uploaderName));
            _projectDocsTable->setItem(row, 3, new QTableWidgetItem(node.name));
            _projectDocsTable->setItem(row, 4, new QTableWidgetItem(file.uploadTime.toString("yyyy-MM-dd HH:mm")));
            
            loadedFileIds.insert(file.id);
        }
    }
    
    // 更新按钮权限
    updateUIBasedOnRole();
}

void ProjectManagementWidget::updateUIBasedOnRole()
{
    bool isAdmin = (_currentUser.role == UserRole::ADMINISTRATOR);
    bool isManager = (_currentUser.role == UserRole::PROJECTMANAGER);
    bool isCurrentProjectManager = (isManager && _currentProject.managerId == _currentUser.id);
    
    // 项目列表视图权限
    _addProjectButton->setEnabled(isAdmin || isManager);
    _editProjectButton->setEnabled(isAdmin || isManager);
    _deleteProjectButton->setEnabled(isAdmin);
    
    // 项目详情视图权限
    _assignUsersButton->setEnabled(isAdmin || isCurrentProjectManager);
    _addNodeButton->setEnabled(isAdmin || isCurrentProjectManager);
    _editNodeButton->setEnabled(isAdmin || isCurrentProjectManager);
    _deleteNodeButton->setEnabled(isAdmin || isCurrentProjectManager);
    
    // 项目文档视图权限
    _addDocButton->setEnabled(isAdmin || isManager || (_currentUser.role == UserRole::NORMALUSER));
    _removeDocButton->setEnabled(isAdmin || isCurrentProjectManager);
}

void ProjectManagementWidget::onAddProject()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && _currentUser.role != UserRole::PROJECTMANAGER) {
        QMessageBox::warning(this, "权限不足", "您没有创建项目的权限。");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("新建项目");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTabWidget* tabWidget = new QTabWidget();
    
    // 基本信息选项卡
    QWidget* basicTab = new QWidget();
    QFormLayout* formLayout = new QFormLayout(basicTab);
    
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("项目名称");
    formLayout->addRow("项目名称:", nameEdit);
    
    QTextEdit* descEdit = new QTextEdit();
    descEdit->setPlaceholderText("项目描述");
    formLayout->addRow("项目描述:", descEdit);
    
    QDateEdit* dateEdit = new QDateEdit(QDate::currentDate().addMonths(1));
    dateEdit->setCalendarPopup(true);
    dateEdit->setMinimumDate(QDate::currentDate());
    formLayout->addRow("预计完成日期:", dateEdit);
    
    tabWidget->addTab(basicTab, "基本信息");
    
    // 项目成员选项卡
    QWidget* membersTab = new QWidget();
    QVBoxLayout* membersLayout = new QVBoxLayout(membersTab);
    
    QTableWidget* usersTable = new QTableWidget();
    usersTable->setColumnCount(3);
    QStringList headers;
    headers << "ID" << "用户名" << "选择";
    usersTable->setHorizontalHeaderLabels(headers);
    usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // 获取所有用户
    QVector<User> allUsers = DataBaseManagement::Instance()->GetAllUsers();
    
    for(const User& user : allUsers) {
        // 跳过当前用户（项目经理）
        if(user.id == _currentUser.id) {
            continue;
        }
        
        int row = usersTable->rowCount();
        usersTable->insertRow(row);
        
        usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        usersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
        
        QCheckBox* checkBox = new QCheckBox();
        usersTable->setCellWidget(row, 2, checkBox);
    }
    
    membersLayout->addWidget(usersTable);
    tabWidget->addTab(membersTab, "项目成员");
    
    layout->addWidget(tabWidget);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString desc = descEdit->toPlainText().trimmed();
        QDateTime estimatedCompleteTime = dateEdit->dateTime();
        
        if(name.isEmpty()) {
            QMessageBox::warning(this, "错误", "项目名称不能为空。");
            return;
        }
        
        Project newProject;
        newProject.name = name;
        newProject.description = desc;
        newProject.managerId = _currentUser.id;
        newProject.managerName = _currentUser.userName;
        newProject.createTime = QDateTime::currentDateTime();
        newProject.estimatedCompleteTime = estimatedCompleteTime;
        newProject.isCompleted = false;
        
        // 添加项目
        int projectId = DataBaseManagement::Instance()->AddProject(newProject);
        if(projectId > 0) {
            // 收集选中的项目成员
            QVector<int> memberIds;
            for(int row = 0; row < usersTable->rowCount(); row++) {
                QCheckBox* checkBox = qobject_cast<QCheckBox*>(usersTable->cellWidget(row, 2));
                if(checkBox && checkBox->isChecked()) {
                    int userId = usersTable->item(row, 0)->text().toInt();
                    memberIds.append(userId);
                }
            }
            
            // 添加项目成员关联
            if(!memberIds.isEmpty()) {
                if(DataBaseManagement::Instance()->AssignUsersToProject(projectId, memberIds)) {
                    QMessageBox::information(this, "成功", QString("项目创建成功，已添加 %1 名成员。").arg(memberIds.size()));
                } else {
                    QMessageBox::warning(this, "警告", "项目创建成功，但添加项目成员失败。");
                }
            } else {
                QMessageBox::information(this, "成功", "项目创建成功。");
            }
            
            loadProjectData();
        } else {
            QMessageBox::critical(this, "错误", "项目创建失败。");
        }
    }
}

void ProjectManagementWidget::onEditProject()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && _currentUser.role != UserRole::PROJECTMANAGER) {
        QMessageBox::warning(this, "权限不足", "您没有编辑项目的权限。");
        return;
    }
    
    QList<QTableWidgetItem*> selectedItems = _projectsTable->selectedItems();
    if(selectedItems.count() == 0) {
        QMessageBox::warning(this, "提示", "请先选择要编辑的项目。");
        return;
    }
    
    int row = selectedItems.first()->row();
    int projectId = _projectsTable->item(row, 0)->text().toInt();
    
    Project project = DataBaseManagement::Instance()->GetProjectById(projectId);
    if(project.id == -1) {
        QMessageBox::warning(this, "错误", "无法加载项目信息，项目可能已被删除。");
        loadProjectData();
        return;
    }
    
    // 检查权限：只有管理员或项目经理可以编辑
    if(_currentUser.role != UserRole::ADMINISTRATOR && project.managerId != _currentUser.id) {
        QMessageBox::warning(this, "权限不足", "您只能编辑自己负责的项目。");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("编辑项目");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTabWidget* tabWidget = new QTabWidget();
    
    // 基本信息选项卡
    QWidget* basicTab = new QWidget();
    QFormLayout* formLayout = new QFormLayout(basicTab);
    
    QLineEdit* nameEdit = new QLineEdit(project.name);
    formLayout->addRow("项目名称:", nameEdit);
    
    QTextEdit* descEdit = new QTextEdit();
    descEdit->setText(project.description);
    formLayout->addRow("项目描述:", descEdit);
    
    QDateEdit* dateEdit = new QDateEdit(project.estimatedCompleteTime.date());
    dateEdit->setCalendarPopup(true);
    formLayout->addRow("预计完成日期:", dateEdit);
    
    QCheckBox* completedCheck = new QCheckBox("项目已完成");
    completedCheck->setChecked(project.isCompleted);
    formLayout->addRow("", completedCheck);
    
    tabWidget->addTab(basicTab, "基本信息");
    
    // 项目成员选项卡
    QWidget* membersTab = new QWidget();
    QVBoxLayout* membersLayout = new QVBoxLayout(membersTab);
    
    QTableWidget* usersTable = new QTableWidget();
    usersTable->setColumnCount(3);
    QStringList headers;
    headers << "ID" << "用户名" << "选择";
    usersTable->setHorizontalHeaderLabels(headers);
    usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // 获取所有用户
    QVector<User> allUsers = DataBaseManagement::Instance()->GetAllUsers();
    // 获取已分配给项目的用户
    QVector<User> projectUsers = DataBaseManagement::Instance()->GetProjectUsers(projectId);
    
    for(const User& user : allUsers) {
        // 跳过当前项目经理
        if(user.id == project.managerId) {
            continue;
        }
        
        int row = usersTable->rowCount();
        usersTable->insertRow(row);
        
        usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        usersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
        
        QCheckBox* checkBox = new QCheckBox();
        // 检查用户是否已分配到项目
        for(const User& projUser : projectUsers) {
            if(projUser.id == user.id) {
                checkBox->setChecked(true);
                break;
            }
        }
        usersTable->setCellWidget(row, 2, checkBox);
    }
    
    membersLayout->addWidget(usersTable);
    tabWidget->addTab(membersTab, "项目成员");
    
    layout->addWidget(tabWidget);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString desc = descEdit->toPlainText().trimmed();
        QDateTime estimatedCompleteTime = dateEdit->dateTime();
        bool isCompleted = completedCheck->isChecked();
        
        if(name.isEmpty()) {
            QMessageBox::warning(this, "错误", "项目名称不能为空。");
            return;
        }
        
        project.name = name;
        project.description = desc;
        project.estimatedCompleteTime = estimatedCompleteTime;
        project.isCompleted = isCompleted;
        
        bool projectUpdated = DataBaseManagement::Instance()->UpdateProject(project);
        
        // 收集选中的项目成员
        QVector<int> memberIds;
        for(int row = 0; row < usersTable->rowCount(); row++) {
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(usersTable->cellWidget(row, 2));
            if(checkBox && checkBox->isChecked()) {
                int userId = usersTable->item(row, 0)->text().toInt();
                memberIds.append(userId);
            }
        }
        
        bool membersUpdated = DataBaseManagement::Instance()->UpdateProjectUsers(projectId, memberIds);
        
        if(projectUpdated && membersUpdated) {
            QMessageBox::information(this, "成功", "项目信息与成员更新成功。");
        } else if(projectUpdated) {
            QMessageBox::warning(this, "警告", "项目信息更新成功，但项目成员更新失败。");
        } else {
            QMessageBox::critical(this, "错误", "项目更新失败。");
        }
        
        loadProjectData();
    }
}

void ProjectManagementWidget::onDeleteProject()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR) {
        QMessageBox::warning(this, "权限不足", "只有管理员可以删除项目。");
        return;
    }
    
    QList<QTableWidgetItem*> selectedItems = _projectsTable->selectedItems();
    if(selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要删除的项目。");
        return;
    }
    
    int row = selectedItems.first()->row();
    int projectId = _projectsTable->item(row, 0)->text().toInt();
    QString projectName = _projectsTable->item(row, 1)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", 
        QString("确定要删除项目 \"%1\" 吗？\n\n注意：删除项目将同时删除所有相关的项目节点和文件。").arg(projectName),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if(reply == QMessageBox::Yes) {
        if(DataBaseManagement::Instance()->DeleteProject(projectId)) {
            QMessageBox::information(this, "成功", "项目已成功删除。");
            loadProjectData();
        } else {
            QMessageBox::critical(this, "错误", "删除项目失败。");
        }
    }
}

void ProjectManagementWidget::onSearchProject()
{
    loadProjectData();
}

void ProjectManagementWidget::onRefreshProjects()
{
    _searchBox->clear();
    _statusFilter->setCurrentIndex(0);
    loadProjectData();
}

void ProjectManagementWidget::onViewProjectDetail(int projectId)
{
    loadProjectDetail(projectId);
    _stackedWidget->setCurrentIndex(1);
}

void ProjectManagementWidget::onBackToList()
{
    _stackedWidget->setCurrentIndex(0);
    loadProjectData();
}

void ProjectManagementWidget::onAssignUsers()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有分配项目成员的权限。");
        return;
    }
    
    // 创建用户选择对话框
    QDialog dialog(this);
    dialog.setWindowTitle("分配项目成员");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 获取所有用户
    QVector<User> allUsers = DataBaseManagement::Instance()->GetAllUsers();
    // 获取已分配给项目的用户
    QVector<User> projectUsers = DataBaseManagement::Instance()->GetProjectUsers(_currentProject.id);
    
    QTableWidget* usersTable = new QTableWidget();
    usersTable->setColumnCount(3);
    QStringList headers;
    headers << "ID" << "用户名" << "分配到项目";
    usersTable->setHorizontalHeaderLabels(headers);
    usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // 填充用户列表
    for(const User& user : allUsers) {
        // 跳过项目经理（项目经理默认为项目成员）
        if(user.id == _currentProject.managerId) {
            continue;
        }
        
        int row = usersTable->rowCount();
        usersTable->insertRow(row);
        
        usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        usersTable->setItem(row, 1, new QTableWidgetItem(user.userName));
        
        QCheckBox* checkBox = new QCheckBox();
        // 检查用户是否已分配到项目
        for(const User& projUser : projectUsers) {
            if(projUser.id == user.id) {
                checkBox->setChecked(true);
                break;
            }
        }
        usersTable->setCellWidget(row, 2, checkBox);
    }
    
    layout->addWidget(usersTable);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        // 收集已选择的用户
        QVector<int> selectedUserIds;
        
        for(int row = 0; row < usersTable->rowCount(); row++) {
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(usersTable->cellWidget(row, 2));
            if(checkBox && checkBox->isChecked()) {
                int userId = usersTable->item(row, 0)->text().toInt();
                selectedUserIds.append(userId);
            }
        }
        
        qDebug() << "更新项目成员，项目ID: " << _currentProject.id << ", 选择的成员数: " << selectedUserIds.size();
        
        // 更新项目成员 - 确保项目经理总是项目成员
        if(!selectedUserIds.contains(_currentProject.managerId)) {
            selectedUserIds.append(_currentProject.managerId);
        }
        
        // 更新项目成员
        if(DataBaseManagement::Instance()->UpdateProjectUsers(_currentProject.id, selectedUserIds)) {
            QMessageBox::information(this, "成功", QString("已更新项目成员，共 %1 名成员。").arg(selectedUserIds.size()));
            loadProjectDetail(_currentProject.id); // 重新加载项目详情以显示更新
        } else {
            QMessageBox::critical(this, "错误", "更新项目成员失败。");
        }
    }
}

// 项目节点管理
void ProjectManagementWidget::onAddNode()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有添加项目节点的权限。");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("添加项目节点");
    dialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();
    
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("节点名称");
    formLayout->addRow("节点名称:", nameEdit);
    
    QTextEdit* descEdit = new QTextEdit();
    descEdit->setPlaceholderText("节点描述");
    formLayout->addRow("节点描述:", descEdit);
    
    // 父节点选择（可选）
    QComboBox* parentNodeCombo = new QComboBox();
    parentNodeCombo->addItem("无（顶级节点）", -1);
    
    QVector<ProjectNode> nodes = DataBaseManagement::Instance()->GetProjectNodes(_currentProject.id);
    for(const ProjectNode& node : nodes) {
        parentNodeCombo->addItem(node.name, node.id);
    }
    
    formLayout->addRow("父节点:", parentNodeCombo);
    
    QDateEdit* dateEdit = new QDateEdit(QDate::currentDate().addMonths(1));
    dateEdit->setCalendarPopup(true);
    dateEdit->setMinimumDate(QDate::currentDate());
    formLayout->addRow("预计完成日期:", dateEdit);
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString desc = descEdit->toPlainText().trimmed();
        int parentId = parentNodeCombo->currentData().toInt();
        QDateTime estimatedCompletionTime = dateEdit->dateTime();
        
        if(name.isEmpty()) {
            QMessageBox::warning(this, "错误", "节点名称不能为空。");
            return;
        }
        
        ProjectNode newNode;
        newNode.projectId = _currentProject.id;
        newNode.name = name;
        newNode.description = desc;
        newNode.parentId = (parentId == -1) ? 0 : parentId;
        newNode.estimatedCompletionTime = estimatedCompletionTime;
        newNode.isCompleted = false;
        
        if(DataBaseManagement::Instance()->AddProjectNode(newNode)) {
            QMessageBox::information(this, "成功", "项目节点创建成功。");
            loadProjectDetail(_currentProject.id);
        } else {
            QMessageBox::critical(this, "错误", "项目节点创建失败。");
        }
    }
}

void ProjectManagementWidget::onEditNode()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有编辑项目节点的权限。");
        return;
    }
    
    QList<QTableWidgetItem*> selectedItems = _projectNodesTable->selectedItems();
    if(selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要编辑的项目节点。");
        return;
    }
    
    int row = selectedItems.first()->row();
    int nodeId = _projectNodesTable->item(row, 0)->text().toInt();
    
    // 获取节点信息
    QVector<ProjectNode> nodes = DataBaseManagement::Instance()->GetProjectNodes(_currentProject.id);
    ProjectNode currentNode;
    
    for(const ProjectNode& node : nodes) {
        if(node.id == nodeId) {
            currentNode = node;
            break;
        }
    }
    
    if(currentNode.id == 0) {
        QMessageBox::warning(this, "错误", "无法加载节点信息。");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("编辑项目节点");
    dialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();
    
    QLineEdit* nameEdit = new QLineEdit(currentNode.name);
    formLayout->addRow("节点名称:", nameEdit);
    
    QTextEdit* descEdit = new QTextEdit();
    descEdit->setText(currentNode.description);
    formLayout->addRow("节点描述:", descEdit);
    
    // 父节点选择
    QComboBox* parentNodeCombo = new QComboBox();
    parentNodeCombo->addItem("无（顶级节点）", -1);
    
    for(const ProjectNode& node : nodes) {
        if(node.id != nodeId) { // 不能选择自己作为父节点
            parentNodeCombo->addItem(node.name, node.id);
            if(node.id == currentNode.parentId) {
                parentNodeCombo->setCurrentIndex(parentNodeCombo->count() - 1);
            }
        }
    }
    
    formLayout->addRow("父节点:", parentNodeCombo);
    
    QDateEdit* dateEdit = new QDateEdit(currentNode.estimatedCompletionTime.date());
    dateEdit->setCalendarPopup(true);
    formLayout->addRow("预计完成日期:", dateEdit);
    
    QCheckBox* completedCheck = new QCheckBox("节点已完成");
    completedCheck->setChecked(currentNode.isCompleted);
    formLayout->addRow("", completedCheck);
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString desc = descEdit->toPlainText().trimmed();
        int parentId = parentNodeCombo->currentData().toInt();
        QDateTime estimatedCompletionTime = dateEdit->dateTime();
        bool isCompleted = completedCheck->isChecked();
        
        if(name.isEmpty()) {
            QMessageBox::warning(this, "错误", "节点名称不能为空。");
            return;
        }
        
        currentNode.name = name;
        currentNode.description = desc;
        currentNode.parentId = (parentId == -1) ? 0 : parentId;
        currentNode.estimatedCompletionTime = estimatedCompletionTime;
        currentNode.isCompleted = isCompleted;
        
        if(DataBaseManagement::Instance()->UpdateProjectNode(currentNode)) {
            QMessageBox::information(this, "成功", "项目节点更新成功。");
            loadProjectDetail(_currentProject.id);
        } else {
            QMessageBox::critical(this, "错误", "项目节点更新失败。");
        }
    }
}

void ProjectManagementWidget::onDeleteNode()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有删除项目节点的权限。");
        return;
    }
    
    QList<QTableWidgetItem*> selectedItems = _projectNodesTable->selectedItems();
    if(selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要删除的项目节点。");
        return;
    }
    
    int row = selectedItems.first()->row();
    int nodeId = _projectNodesTable->item(row, 0)->text().toInt();
    QString nodeName = _projectNodesTable->item(row, 1)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", 
        QString("确定要删除项目节点 \"%1\" 吗？\n\n注意：删除节点可能会影响项目结构。").arg(nodeName),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if(reply == QMessageBox::Yes) {
        if(DataBaseManagement::Instance()->DeleteProjectNode(nodeId)) {
            QMessageBox::information(this, "成功", "项目节点已成功删除。");
            loadProjectDetail(_currentProject.id);
        } else {
            QMessageBox::critical(this, "错误", "删除项目节点失败。");
        }
    }
}

void ProjectManagementWidget::onNodeStatusChanged(int nodeId, bool completed)
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       _currentUser.role != UserRole::PROJECTMANAGER && 
       _currentUser.role != UserRole::NORMALUSER) {
        QMessageBox::warning(this, "权限不足", "您没有更改节点状态的权限。");
        return;
    }
    
    // 获取节点信息
    QVector<ProjectNode> nodes = DataBaseManagement::Instance()->GetProjectNodes(_currentProject.id);
    ProjectNode currentNode;
    
    for(const ProjectNode& node : nodes) {
        if(node.id == nodeId) {
            currentNode = node;
            break;
        }
    }
    
    if(currentNode.id == 0) {
        QMessageBox::warning(this, "错误", "无法加载节点信息。");
        return;
    }
    
    currentNode.isCompleted = completed;
    
    if(DataBaseManagement::Instance()->UpdateProjectNode(currentNode)) {
        loadProjectDetail(_currentProject.id);
    } else {
        QMessageBox::critical(this, "错误", "更新节点状态失败。");
    }
}

// 项目文档管理方法
void ProjectManagementWidget::onAddDocument()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       _currentUser.role != UserRole::PROJECTMANAGER && 
       _currentUser.role != UserRole::NORMALUSER) {
        QMessageBox::warning(this, "权限不足", "您没有添加项目文档的权限。");
        return;
    }
    
    // 首先检查项目是否有节点
    QVector<ProjectNode> projectNodes = DataBaseManagement::Instance()->GetProjectNodes(_currentProject.id);
    if(projectNodes.isEmpty()) {
        QMessageBox::warning(this, "无法添加文档", "该项目还没有创建任何节点，请先创建项目节点。");
        return;
    }
    
    // 显示可用文档的对话框
    QDialog dialog(this);
    dialog.setWindowTitle("添加文档到项目节点");
    dialog.setMinimumWidth(600);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 添加节点选择下拉框
    QFormLayout* formLayout = new QFormLayout();
    QComboBox* nodeComboBox = new QComboBox();
    for(const ProjectNode& node : projectNodes) {
        nodeComboBox->addItem(node.name, node.id);
    }
    formLayout->addRow("选择项目节点:", nodeComboBox);
    layout->addLayout(formLayout);
    
    // 获取所有文件
    QVector<FileInfo> allFiles = DataBaseManagement::Instance()->GetAllFiles();
    
    // 创建已关联文件ID集合，过滤掉已经绑定的文件
    QSet<int> nodeFileIds;
    // 获取所选节点的文件ID，使用当前选择的第一个节点
    int currentNodeId = nodeComboBox->currentData().toInt();
    QVector<FileInfo> nodeFiles = DataBaseManagement::Instance()->GetNodeFiles(currentNodeId);
    for(const FileInfo& file : nodeFiles) {
        nodeFileIds.insert(file.id);
    }
    
    QVector<FileInfo> availableFiles;
    // 过滤出未分配给节点或者状态为正常的文件
    for(const FileInfo& file : allFiles) {
        if(!nodeFileIds.contains(file.id) && file.status == FileStatus::NORMAL) {
            availableFiles.append(file);
        }
    }
    
    QTableWidget* docsTable = new QTableWidget();
    docsTable->setColumnCount(5);
    QStringList headers;
    headers << "ID" << "文档名称" << "上传者" << "上传时间" << "选择";
    docsTable->setHorizontalHeaderLabels(headers);
    docsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    docsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    docsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // 填充文档列表
    for(const FileInfo& doc : availableFiles) {
        int row = docsTable->rowCount();
        docsTable->insertRow(row);
        
        docsTable->setItem(row, 0, new QTableWidgetItem(QString::number(doc.id)));
        docsTable->setItem(row, 1, new QTableWidgetItem(doc.fileName));
        docsTable->setItem(row, 2, new QTableWidgetItem(doc.uploaderName));
        docsTable->setItem(row, 3, new QTableWidgetItem(doc.uploadTime.toString("yyyy-MM-dd HH:mm")));
        
        QCheckBox* checkBox = new QCheckBox();
        docsTable->setCellWidget(row, 4, checkBox);
    }
    
    layout->addWidget(docsTable);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        // 获取选中的节点ID
        int nodeId = nodeComboBox->currentData().toInt();
        if(nodeId <= 0) {
            QMessageBox::warning(this, "错误", "请选择一个有效的项目节点。");
            return;
        }
        
        // 收集已选择的文档
        QVector<int> selectedFileIds;
        
        for(int row = 0; row < docsTable->rowCount(); row++) {
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(docsTable->cellWidget(row, 4));
            if(checkBox && checkBox->isChecked()) {
                int fileId = docsTable->item(row, 0)->text().toInt();
                selectedFileIds.append(fileId);
            }
        }
        
        if(!selectedFileIds.isEmpty()) {
            if(DataBaseManagement::Instance()->AssignFilesToNode(nodeId, selectedFileIds)) {
                QMessageBox::information(this, "成功", QString("已添加 %1 个文档到项目节点。").arg(selectedFileIds.size()));
                loadProjectDetail(_currentProject.id); // 重新加载项目详情以显示更新
            } else {
                QMessageBox::critical(this, "错误", "添加文档到项目节点失败。");
            }
        } else {
            QMessageBox::warning(this, "提示", "未选择任何文档。");
        }
    }
}

void ProjectManagementWidget::onRemoveDocument()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有从项目中移除文档的权限。");
        return;
    }
    
    QList<QTableWidgetItem*> selectedItems = _projectDocsTable->selectedItems();
    if(selectedItems.count() == 0) {
        QMessageBox::warning(this, "提示", "请先选择要移除的文档。");
        return;
    }
    
    int row = selectedItems.first()->row();
    int fileId = _projectDocsTable->item(row, 0)->text().toInt();
    QString fileName = _projectDocsTable->item(row, 1)->text();
    
    // 获取所有项目节点
    QVector<ProjectNode> projectNodes = DataBaseManagement::Instance()->GetProjectNodes(_currentProject.id);
    if(projectNodes.isEmpty()) {
        QMessageBox::warning(this, "警告", "该项目没有任何节点。");
        return;
    }
    
    // 创建对话框，让用户选择从哪个节点移除文档
    QDialog dialog(this);
    dialog.setWindowTitle("选择要移除文档的节点");
    dialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* label = new QLabel(QString("确定要移除文档 \"%1\" 吗？\n请选择要从中移除文档的节点：").arg(fileName));
    layout->addWidget(label);
    
    QComboBox* nodeComboBox = new QComboBox();
    for(const ProjectNode& node : projectNodes) {
        // 检查该节点是否关联了该文档
        QVector<FileInfo> nodeFiles = DataBaseManagement::Instance()->GetNodeFiles(node.id);
        bool hasFile = false;
        for(const FileInfo& file : nodeFiles) {
            if(file.id == fileId) {
                hasFile = true;
                break;
            }
        }
        
        if(hasFile) {
            nodeComboBox->addItem(node.name, node.id);
        }
    }
    
    if(nodeComboBox->count() == 0) {
        QMessageBox::warning(this, "警告", "没有任何节点关联了该文档。");
        return;
    }
    
    layout->addWidget(nodeComboBox);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        int nodeId = nodeComboBox->currentData().toInt();
        
        // 获取节点当前关联的所有文件
        QVector<FileInfo> nodeFiles = DataBaseManagement::Instance()->GetNodeFiles(nodeId);
        QVector<int> fileIds;
        for(const FileInfo& file : nodeFiles) {
            if(file.id != fileId) { // 排除要移除的文件
                fileIds.append(file.id);
            }
        }
        
        // 更新节点关联的文件
        if(DataBaseManagement::Instance()->AssignFilesToNode(nodeId, fileIds)) {
            QMessageBox::information(this, "成功", QString("文档已从节点 %1 中移除。").arg(nodeComboBox->currentText()));
            loadProjectDetail(_currentProject.id);
        } else {
            QMessageBox::warning(this, "警告", "移除文档失败，请稍后重试。");
        }
    }
}

void ProjectManagementWidget::openDocument(int fileId)
{
    // 使用GetAllFiles获取所有文件，然后找出指定ID的文件
    QVector<FileInfo> allFiles = DataBaseManagement::Instance()->GetAllFiles();
    FileInfo targetFile;
    
    for(const FileInfo& file : allFiles) {
        if(file.id == fileId) {
            targetFile = file;
            break;
        }
    }
    
    if(targetFile.id == 0) {
        QMessageBox::warning(this, "错误", "无法打开文档，文档可能已被删除。");
        return;
    }
    
    // 调用系统默认程序打开文档
    QDesktopServices::openUrl(QUrl::fromLocalFile(targetFile.filePath));
}

void ProjectManagementWidget::onAssociateFiles()
{
    if(_currentUser.role != UserRole::ADMINISTRATOR && 
       (_currentUser.role != UserRole::PROJECTMANAGER || _currentProject.managerId != _currentUser.id)) {
        QMessageBox::warning(this, "权限不足", "您没有关联文件到项目的权限。");
        return;
    }
    
    // 创建文件选择对话框
    QDialog dialog(this);
    dialog.setWindowTitle("关联文件到项目");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 获取所有文件
    QVector<FileInfo> allFiles = DataBaseManagement::Instance()->GetAllFiles();
    // 获取已关联到项目的文件
    QVector<FileInfo> projectFiles = DataBaseManagement::Instance()->GetProjectFiles(_currentProject.id);
    
    // 创建一个集合存储已关联文件的ID，方便快速查找
    QSet<int> projectFileIds;
    for (const FileInfo& file : projectFiles) {
        projectFileIds.insert(file.id);
    }
    
    QTableWidget* filesTable = new QTableWidget();
    filesTable->setColumnCount(4);
    QStringList headers;
    headers << "ID" << "文件名" << "类型" << "关联到项目";
    filesTable->setHorizontalHeaderLabels(headers);
    filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    filesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // 填充文件列表
    for(const FileInfo& file : allFiles) {
        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        
        filesTable->setItem(row, 0, new QTableWidgetItem(QString::number(file.id)));
        filesTable->setItem(row, 1, new QTableWidgetItem(file.fileName));
        
        QString fileType;
        if(file.fileType == FileType::DOCUMENT) {
            fileType = "文档";
        } else {
            fileType = "其他";
        }
        filesTable->setItem(row, 2, new QTableWidgetItem(fileType));
        
        QCheckBox* checkBox = new QCheckBox();
        // 检查文件是否已关联到项目
        checkBox->setChecked(projectFileIds.contains(file.id));
        filesTable->setCellWidget(row, 3, checkBox);
    }
    
    layout->addWidget(filesTable);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if(dialog.exec() == QDialog::Accepted) {
        // 收集已选择的文件
        QVector<int> selectedFileIds;
        
        for(int row = 0; row < filesTable->rowCount(); row++) {
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(filesTable->cellWidget(row, 3));
            if(checkBox && checkBox->isChecked()) {
                int fileId = filesTable->item(row, 0)->text().toInt();
                selectedFileIds.append(fileId);
            }
        }
        
        qDebug() << "更新项目关联文件，项目ID: " << _currentProject.id << ", 选择的文件数: " << selectedFileIds.size();
        
        // 更新项目关联文件
        if(DataBaseManagement::Instance()->UpdateProjectFiles(_currentProject.id, selectedFileIds)) {
            QMessageBox::information(this, "成功", QString("已更新项目关联文件，共 %1 个文件。").arg(selectedFileIds.size()));
            loadProjectDetail(_currentProject.id); // 重新加载项目详情以显示更新
        } else {
            QMessageBox::critical(this, "错误", "更新项目关联文件失败。");
        }
    }
}

void ProjectManagementWidget::filterProjects()
{
    loadProjectData();
}

void ProjectManagementWidget::searchProjects()
{
    loadProjectData();
} 
