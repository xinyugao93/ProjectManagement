#ifndef DBMODELS_H
#define DBMODELS_H

#include <QString>
#include <QDateTime>

enum class UserRole
{
    ADMINISTRATOR,
    PROJECTMANAGER,
    NORMALUSER
};

struct User
{
    int id;
    QString userName;
    QString password;
    UserRole role;
    QDateTime createTime;
};

// 文件类型枚举
enum class FileType
{
    DOCUMENT,      // 文档文件
    OTHER          // 其他类型
};

// 文件状态枚举
enum class FileStatus
{
    NORMAL,        // 正常状态
    DELETED,       // 已删除（回收站）
    ARCHIVED       // 已归档
};

// 文件结构体
struct FileInfo
{
    int id;
    QString fileName;
    QString filePath;
    QString fileExtension;
    qint64 fileSize;
    int uploaderId;
    QString uploaderName;
    QDateTime uploadTime;
    FileType fileType;
    FileStatus status;
    int projectId;
    bool isProcessDocument; // 是否为过程文档
};

// 项目结构体
struct Project
{
    int id;
    QString name;
    QString description;
    int managerId;
    QString managerName;
    QDateTime createTime;
    QDateTime estimatedCompleteTime;
    bool isCompleted;
};

// 项目节点结构体
struct ProjectNode
{
    int id;
    int projectId;
    QString name;
    QString description;
    int parentId;
    QDateTime creationTime;
    QDateTime estimatedCompletionTime;
    bool isCompleted;
};

#endif // DBMODELS_H
