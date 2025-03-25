#ifndef DATABSEMANAGEMENT_H
#define DATABSEMANAGEMENT_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QVector>
#include "DBmodels.h"

class DataBaseManagement : public QObject
{
    Q_OBJECT
public:
    DataBaseManagement(const DataBaseManagement&) = delete;

    DataBaseManagement(DataBaseManagement&&) = delete;

    DataBaseManagement& operator=(const DataBaseManagement&) = delete;

    DataBaseManagement& operator=(DataBaseManagement&&) = delete;

    ~DataBaseManagement();

    bool Initialize();

    static DataBaseManagement* Instance();

    // 用户相关方法
    User GetUserbyUserName(const QString& userName);
    QVector<User> GetAllUsers();
    bool AddUser(const User& user);
    bool UpdateUser(const User& user);
    bool DeleteUser(int userId);

    // 文件相关方法
    QVector<FileInfo> GetAllFiles(FileStatus status = FileStatus::NORMAL);
    QVector<FileInfo> GetFilesByProject(int projectId, FileStatus status = FileStatus::NORMAL);
    QVector<FileInfo> GetProcessDocuments();
    bool AddFile(const FileInfo& file);
    bool UpdateFile(const FileInfo& file);
    bool DeleteFile(int fileId, bool permanent = false);
    bool RestoreFile(int fileId);

    // 项目相关方法
    QVector<Project> GetAllProjects();
    Project GetProjectById(int projectId);
    int AddProject(const Project& project);
    bool UpdateProject(const Project& project);
    bool DeleteProject(int projectId);
    
    // 项目成员相关方法
    QVector<User> GetProjectUsers(int projectId);
    bool AssignUsersToProject(int projectId, const QVector<int>& userIds);
    bool UpdateProjectUsers(int projectId, const QVector<int>& userIds);
    
    // 项目文件相关方法
    bool UpdateProjectFiles(int projectId, const QVector<int>& fileIds);

    // 项目节点相关方法
    QVector<ProjectNode> GetProjectNodes(int projectId);
    bool AddProjectNode(const ProjectNode& node);
    bool UpdateProjectNode(const ProjectNode& node);
    bool DeleteProjectNode(int nodeId);

    // 项目文件关联相关方法
    bool AssignFilesToProject(int projectId, const QVector<int>& fileIds);
    QVector<FileInfo> GetProjectFiles(int projectId, FileStatus status = FileStatus::NORMAL);

private:
    explicit DataBaseManagement(QObject* parent = nullptr);

    bool CreateTables();
    bool CreateUserTable();
    bool CreateFileTable();
    bool CreateProjectTable();
    bool CreateProjectNodeTable();
    bool CreateProjectUserTable();
    bool CreateProjectFileTable();

    bool InsertDefaultData();

private:
    QSqlDatabase _db;

};

#endif // DATABSEMANAGEMENT_H
