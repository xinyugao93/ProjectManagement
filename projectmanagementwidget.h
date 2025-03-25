#ifndef PROJECTMANAGEMENTWIDGET_H
#define PROJECTMANAGEMENTWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDate>
#include <QMenu>
#include <QAction>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include "DBModels.h"

class ProjectManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectManagementWidget(QWidget *parent = nullptr);
    ~ProjectManagementWidget();

    void setCurrentUser(const User &user);

private:
    // UI初始化
    void setupUI();
    void setupProjectListView();
    void setupProjectDetailView();
    
    // 数据加载
    void loadProjectData(const QString& status = "", const QString& search = "");
    void loadProjectDetail(int projectId);
    void updateUIBasedOnRole();
    
    // 用户操作响应
    void onAddProject();
    void onEditProject();
    void onDeleteProject();
    void onSearchProject();
    void onRefreshProjects();
    void onViewProjectDetail(int projectId);
    void onBackToList();
    
    // 项目成员管理
    void onAssignUsers();
    void onAssociateFiles();
    
    // 项目节点管理
    void onAddNode();
    void onEditNode();
    void onDeleteNode();
    void onNodeStatusChanged(int nodeId, bool completed);
    
    // 项目文档管理
    void onAddDocument();
    void onRemoveDocument();
    void openDocument(int fileId);
    
    // 其他操作
    void filterProjects();
    void searchProjects();
    void onItemClicked(QTableWidgetItem* item);
    void onNodeClick(QTableWidgetItem* item);
    void onOpenNode(const QModelIndex& index);
    void onManageNodes();
    
    // UI组件
    QStackedWidget* _stackedWidget;
    
    // 项目列表视图
    QWidget* _projectListView;
    QTableWidget* _projectsTable;
    QLineEdit* _searchBox;
    QComboBox* _statusFilter;
    QPushButton* _addProjectButton;
    QPushButton* _editProjectButton;
    QPushButton* _deleteProjectButton;
    
    // 项目详情视图
    QWidget* _projectDetailView;
    QPushButton* _backButton;
    QLabel* _projectNameLabel;
    QLabel* _projectStatusLabel;
    QLabel* _projectDescLabel;
    QTableWidget* _projectMembersTable;
    QPushButton* _assignUsersButton;
    QTableWidget* _projectNodesTable;
    QPushButton* _addNodeButton;
    QPushButton* _editNodeButton;
    QPushButton* _deleteNodeButton;
    QTableWidget* _projectDocsTable;
    QPushButton* _addDocButton;
    QPushButton* _removeDocButton;
    
    // 数据
    User _currentUser;
    Project _currentProject;
};

#endif // PROJECTMANAGEMENTWIDGET_H 
