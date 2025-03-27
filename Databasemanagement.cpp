#include <QDir>
#include <QDebug>
#include "Databasemanagement.h"

DataBaseManagement::DataBaseManagement(QObject* parent) : QObject(parent)
{

}

DataBaseManagement* DataBaseManagement::Instance()
{
    static DataBaseManagement mgr;
    return &mgr;
}

DataBaseManagement::~DataBaseManagement()
{
    if(_db.isOpen())
    {
        _db.close();
    }
}

bool DataBaseManagement::Initialize()
{
    QString dataPath = QDir::currentPath();
    _db = QSqlDatabase::addDatabase("QSQLITE");
    _db.setDatabaseName(dataPath + "/projectmanager.db");

    if(!_db.open())
    {
        qDebug() << "Cannot open database: " << _db.lastError().text();
        return false;
    }

    if(!CreateTables())
    {
        qDebug() << "Failed to create tables";
        return false;
    }

    if(!InsertDefaultData())
    {
        qDebug() << "Failed to insert default values";
        return false;
    }

    qDebug() << "数据库初始化成功，路径: " << dataPath + "/projectmanager.db";
    return true;
}

bool DataBaseManagement::CreateTables()
{
    // 确保按正确的顺序创建表，避免外键约束问题
    if (!CreateUserTable())
    {
        qDebug() << "创建用户表失败";
        return false;
    }
        
    if (!CreateFileTable())
    {
        qDebug() << "创建文件表失败";
        return false;
    }
    
    if (!CreateProjectTable())
    {
        qDebug() << "创建项目表失败";
        return false;
    }
    
    if (!CreateProjectNodeTable())
    {
        qDebug() << "创建项目节点表失败";
        return false;
    }
        
    if (!CreateProjectUserTable())
    {
        qDebug() << "创建项目用户关联表失败";
        return false;
    }
    
    if (!CreateProjectFileTable())
    {
        qDebug() << "创建项目文件关联表失败";
        return false;
    }
    
    if (!CreateNodeFileTable())
    {
        qDebug() << "创建node_file表失败";
        return false;
    }
    
    qDebug() << "所有表创建成功";
    return true;
}

bool DataBaseManagement::CreateUserTable()
{
    QSqlQuery query;

    if(!query.exec("CREATE TABLE IF NOT EXISTS users ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "username TEXT UNIQUE NOT NULL, "
                     "password TEXT NOT NULL, "
                     "role INTEGER NOT NULL CHECK (role IN (0, 1, 2)), "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"))
    {
        qDebug() << "Failed to create user table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateFileTable()
{
    QSqlQuery query;

    if(!query.exec("CREATE TABLE IF NOT EXISTS files ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "file_name TEXT NOT NULL, "
                   "file_path TEXT NOT NULL, "
                   "file_extension TEXT NOT NULL, "
                   "file_size INTEGER NOT NULL, "
                   "uploader_id INTEGER NOT NULL, "
                   "upload_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                   "file_type INTEGER NOT NULL, "
                   "status INTEGER NOT NULL DEFAULT 0, "
                   "project_id INTEGER, "
                   "is_process_document BOOLEAN DEFAULT 0, "
                   "FOREIGN KEY (uploader_id) REFERENCES users (id) ON DELETE CASCADE, "
                   "FOREIGN KEY (project_id) REFERENCES projects (id) ON DELETE SET NULL)"))
    {
        qDebug() << "Failed to create file table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateProjectTable()
{
    QSqlQuery query;

    if(!query.exec("CREATE TABLE IF NOT EXISTS projects ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT NOT NULL, "
                   "description TEXT, "
                   "manager_id INTEGER NOT NULL, "
                   "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                   "estimated_complete_time TIMESTAMP, "
                   "is_completed BOOLEAN DEFAULT 0, "
                   "FOREIGN KEY (manager_id) REFERENCES users (id) ON DELETE CASCADE)"))
    {
        qDebug() << "Failed to create project table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateProjectNodeTable()
{
    QSqlQuery query;

    if(!query.exec("CREATE TABLE IF NOT EXISTS project_nodes ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "project_id INTEGER NOT NULL, "
                   "name TEXT NOT NULL, "
                   "description TEXT, "
                   "parent_id INTEGER, "
                   "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                   "estimated_completion_time TIMESTAMP, "
                   "is_completed BOOLEAN DEFAULT 0, "
                   "FOREIGN KEY (project_id) REFERENCES projects (id) ON DELETE CASCADE, "
                   "FOREIGN KEY (parent_id) REFERENCES project_nodes (id) ON DELETE CASCADE)"))
    {
        qDebug() << "Failed to create project node table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateProjectUserTable()
{
    QSqlQuery query(_db);

    // 创建项目用户关联表
    QString createProjectUserTableSQL = 
        "CREATE TABLE IF NOT EXISTS project_user ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "project_id INTEGER NOT NULL, "
        "user_id INTEGER NOT NULL, "
        "FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE, "
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
        "UNIQUE(project_id, user_id) "
        ")";

    if (!query.exec(createProjectUserTableSQL)) {
        qDebug() << "创建项目用户关联表失败: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateProjectFileTable()
{
    QSqlQuery query(_db);

    // 创建项目文件关联表
    QString createProjectFileTableSQL = 
        "CREATE TABLE IF NOT EXISTS project_file ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "project_id INTEGER NOT NULL, "
        "file_id INTEGER NOT NULL, "
        "FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE, "
        "FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE, "
        "UNIQUE(project_id, file_id) "
        ")";

    if (!query.exec(createProjectFileTableSQL)) {
        qDebug() << "创建项目文件关联表失败: " << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBaseManagement::CreateNodeFileTable()
{
    QSqlQuery query(_db);
    bool success = query.exec("CREATE TABLE IF NOT EXISTS node_file ( "
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "node_id INTEGER NOT NULL, "
                           "file_id INTEGER NOT NULL, "
                           "FOREIGN KEY (node_id) REFERENCES project_nodes (id) ON DELETE CASCADE, "
                           "FOREIGN KEY (file_id) REFERENCES files (id) ON DELETE CASCADE"
                           ")");
    
    if(!success) {
        qDebug() << "Failed to create node_file table: " << query.lastError().text();
    }
    
    return success;
}

bool DataBaseManagement::InsertDefaultData()
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM users WHERE role = 0");
    if(!query.exec() || !query.next())
    {
        qDebug() << "Failed to find Admin" << query.lastError().text();
        return false;
    }

    if(0 == query.value(0).toInt())
    {
        if(!query.exec("INSERT INTO users (username, password, role) "
                        "VALUES ('admin', 'admin123', 0)"))
        {
            qDebug() << "Failed to create default admin account" << query.lastError().text();
            return false;
        }
    }

    return true;
}

User DataBaseManagement::GetUserbyUserName(const QString& userName)
{
    User user;
    user.id = -1;
    QSqlQuery query;
    query.prepare("SELECT id, username, password, role, created_at "
                  "FROM users WHERE username = ?");
    query.bindValue(0, userName);

    if(query.exec() && query.next())
    {
        user.id         = query.value(0).toInt();
        user.userName   = query.value(1).toString();
        user.password   = query.value(2).toString();
        user.role       = static_cast<UserRole>(query.value(3).toInt());
        user.createTime = query.value(4).toDateTime();
    }

    return user;
}

QVector<User> DataBaseManagement::GetAllUsers()
{
    QVector<User> users;
    QSqlQuery query;
    query.prepare("SELECT id, username, password, role, created_at FROM users");
    
    if(query.exec())
    {
        while(query.next())
        {
            User user;
            user.id = query.value(0).toInt();
            user.userName = query.value(1).toString();
            user.password = query.value(2).toString();
            user.role = static_cast<UserRole>(query.value(3).toInt());
            user.createTime = query.value(4).toDateTime();
            users.append(user);
        }
    }
    else
    {
        qDebug() << "Failed to get all users: " << query.lastError().text();
    }
    
    return users;
}

bool DataBaseManagement::AddUser(const User& user)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, role) VALUES (?, ?, ?)");
    query.addBindValue(user.userName);
    query.addBindValue(user.password);
    query.addBindValue(static_cast<int>(user.role));
    
    if(!query.exec())
    {
        qDebug() << "Failed to add user: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::UpdateUser(const User& user)
{
    QSqlQuery query;
    query.prepare("UPDATE users SET username = ?, password = ?, role = ? WHERE id = ?");
    query.addBindValue(user.userName);
    query.addBindValue(user.password);
    query.addBindValue(static_cast<int>(user.role));
    query.addBindValue(user.id);
    
    if(!query.exec())
    {
        qDebug() << "Failed to update user: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::DeleteUser(int userId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE id = ? AND role != 0"); // 防止删除管理员
    query.addBindValue(userId);
    
    if(!query.exec())
    {
        qDebug() << "Failed to delete user: " << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

// 文件相关方法实现
QVector<FileInfo> DataBaseManagement::GetAllFiles(FileStatus status)
{
    QVector<FileInfo> files;
    QSqlQuery query;
    query.prepare("SELECT f.id, f.file_name, f.file_path, f.file_extension, f.file_size, f.uploader_id, "
                 "u.username, f.upload_time, f.file_type, f.status, f.project_id, f.is_process_document "
                 "FROM files f JOIN users u ON f.uploader_id = u.id WHERE f.status = ?");
    query.addBindValue(static_cast<int>(status));
    
    if(query.exec())
    {
        while(query.next())
        {
            FileInfo file;
            file.id = query.value(0).toInt();
            file.fileName = query.value(1).toString();
            file.filePath = query.value(2).toString();
            file.fileExtension = query.value(3).toString();
            file.fileSize = query.value(4).toLongLong();
            file.uploaderId = query.value(5).toInt();
            file.uploaderName = query.value(6).toString();
            file.uploadTime = query.value(7).toDateTime();
            file.fileType = static_cast<FileType>(query.value(8).toInt());
            file.status = static_cast<FileStatus>(query.value(9).toInt());
            file.projectId = query.value(10).toInt();
            file.isProcessDocument = query.value(11).toBool();
            files.append(file);
        }
    }
    else
    {
        qDebug() << "Failed to get files: " << query.lastError().text();
    }
    
    return files;
}

QVector<FileInfo> DataBaseManagement::GetFilesByProject(int projectId, FileStatus status)
{
    QVector<FileInfo> files;
    QSqlQuery query;
    query.prepare("SELECT f.id, f.file_name, f.file_path, f.file_extension, f.file_size, f.uploader_id, "
                 "u.username, f.upload_time, f.file_type, f.status, f.project_id, f.is_process_document "
                 "FROM files f JOIN users u ON f.uploader_id = u.id WHERE f.project_id = ? AND f.status = ?");
    query.addBindValue(projectId);
    query.addBindValue(static_cast<int>(status));
    
    if(query.exec())
    {
        while(query.next())
        {
            FileInfo file;
            file.id = query.value(0).toInt();
            file.fileName = query.value(1).toString();
            file.filePath = query.value(2).toString();
            file.fileExtension = query.value(3).toString();
            file.fileSize = query.value(4).toLongLong();
            file.uploaderId = query.value(5).toInt();
            file.uploaderName = query.value(6).toString();
            file.uploadTime = query.value(7).toDateTime();
            file.fileType = static_cast<FileType>(query.value(8).toInt());
            file.status = static_cast<FileStatus>(query.value(9).toInt());
            file.projectId = query.value(10).toInt();
            file.isProcessDocument = query.value(11).toBool();
            files.append(file);
        }
    }
    else
    {
        qDebug() << "Failed to get files by project: " << query.lastError().text();
    }
    
    return files;
}

QVector<FileInfo> DataBaseManagement::GetProcessDocuments()
{
    QVector<FileInfo> files;
    QSqlQuery query;
    query.prepare("SELECT f.id, f.file_name, f.file_path, f.file_extension, f.file_size, f.uploader_id, "
                 "u.username, f.upload_time, f.file_type, f.status, f.project_id, f.is_process_document "
                 "FROM files f JOIN users u ON f.uploader_id = u.id WHERE f.is_process_document = 1 AND f.status = ?");
    query.addBindValue(static_cast<int>(FileStatus::NORMAL));
    
    if(query.exec())
    {
        while(query.next())
        {
            FileInfo file;
            file.id = query.value(0).toInt();
            file.fileName = query.value(1).toString();
            file.filePath = query.value(2).toString();
            file.fileExtension = query.value(3).toString();
            file.fileSize = query.value(4).toLongLong();
            file.uploaderId = query.value(5).toInt();
            file.uploaderName = query.value(6).toString();
            file.uploadTime = query.value(7).toDateTime();
            file.fileType = static_cast<FileType>(query.value(8).toInt());
            file.status = static_cast<FileStatus>(query.value(9).toInt());
            file.projectId = query.value(10).toInt();
            file.isProcessDocument = query.value(11).toBool();
            files.append(file);
        }
    }
    else
    {
        qDebug() << "Failed to get process documents: " << query.lastError().text();
    }
    
    return files;
}

bool DataBaseManagement::AddFile(const FileInfo& file)
{
    QSqlQuery query;
    query.prepare("INSERT INTO files (file_name, file_path, file_extension, file_size, uploader_id, "
                 "file_type, status, project_id, is_process_document) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(file.fileName);
    query.addBindValue(file.filePath);
    query.addBindValue(file.fileExtension);
    query.addBindValue(file.fileSize);
    query.addBindValue(file.uploaderId);
    query.addBindValue(static_cast<int>(file.fileType));
    query.addBindValue(static_cast<int>(file.status));
    query.addBindValue(file.projectId > 0 ? file.projectId : QVariant());
    query.addBindValue(file.isProcessDocument);
    
    if(!query.exec())
    {
        qDebug() << "Failed to add file: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::UpdateFile(const FileInfo& file)
{
    QSqlQuery query;
    query.prepare("UPDATE files SET file_name = ?, file_path = ?, file_extension = ?, "
                 "file_size = ?, file_type = ?, status = ?, project_id = ?, is_process_document = ? "
                 "WHERE id = ?");
    query.addBindValue(file.fileName);
    query.addBindValue(file.filePath);
    query.addBindValue(file.fileExtension);
    query.addBindValue(file.fileSize);
    query.addBindValue(static_cast<int>(file.fileType));
    query.addBindValue(static_cast<int>(file.status));
    query.addBindValue(file.projectId > 0 ? file.projectId : QVariant());
    query.addBindValue(file.isProcessDocument);
    query.addBindValue(file.id);
    
    if(!query.exec())
    {
        qDebug() << "Failed to update file: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::DeleteFile(int fileId, bool permanent)
{
    QSqlQuery query;
    
    if(permanent)
    {
        // 永久删除文件
        query.prepare("DELETE FROM files WHERE id = ?");
        query.addBindValue(fileId);
    }
    else
    {
        // 标记为已删除状态
        query.prepare("UPDATE files SET status = ? WHERE id = ?");
        query.addBindValue(static_cast<int>(FileStatus::DELETED));
        query.addBindValue(fileId);
    }
    
    if(!query.exec())
    {
        qDebug() << "Failed to delete file: " << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool DataBaseManagement::RestoreFile(int fileId)
{
    QSqlQuery query;
    query.prepare("UPDATE files SET status = ? WHERE id = ? AND status = ?");
    query.addBindValue(static_cast<int>(FileStatus::NORMAL));
    query.addBindValue(fileId);
    query.addBindValue(static_cast<int>(FileStatus::DELETED));
    
    if(!query.exec())
    {
        qDebug() << "Failed to restore file: " << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

// 项目相关方法实现
QVector<Project> DataBaseManagement::GetAllProjects()
{
    QVector<Project> projects;
    QSqlQuery query;
    query.prepare("SELECT p.id, p.name, p.description, p.manager_id, u.username, "
                 "p.create_time, p.estimated_complete_time, p.is_completed "
                 "FROM projects p JOIN users u ON p.manager_id = u.id");
    
    if(query.exec())
    {
        while(query.next())
        {
            Project project;
            project.id = query.value(0).toInt();
            project.name = query.value(1).toString();
            project.description = query.value(2).toString();
            project.managerId = query.value(3).toInt();
            project.managerName = query.value(4).toString();
            project.createTime = query.value(5).toDateTime();
            project.estimatedCompleteTime = query.value(6).toDateTime();
            project.isCompleted = query.value(7).toBool();
            projects.append(project);
        }
    }
    else
    {
        qDebug() << "Failed to get projects: " << query.lastError().text();
    }
    
    return projects;
}

// 项目节点相关方法实现
QVector<ProjectNode> DataBaseManagement::GetProjectNodes(int projectId)
{
    QVector<ProjectNode> nodes;
    QSqlQuery query;
    query.prepare("SELECT id, project_id, name, description, parent_id, create_time, "
                 "estimated_completion_time, is_completed "
                 "FROM project_nodes WHERE project_id = ?");
    query.addBindValue(projectId);
    
    if(query.exec())
    {
        while(query.next())
        {
            ProjectNode node;
            node.id = query.value(0).toInt();
            node.projectId = query.value(1).toInt();
            node.name = query.value(2).toString();
            node.description = query.value(3).toString();
            node.parentId = query.value(4).toInt();
            node.creationTime = query.value(5).toDateTime();
            node.estimatedCompletionTime = query.value(6).toDateTime();
            node.isCompleted = query.value(7).toBool();
            nodes.append(node);
        }
    }
    else
    {
        qDebug() << "Failed to get project nodes: " << query.lastError().text();
    }
    
    return nodes;
}

Project DataBaseManagement::GetProjectById(int projectId)
{
    Project project;
    project.id = -1;
    QSqlQuery query;
    query.prepare("SELECT p.id, p.name, p.description, p.manager_id, u.username, "
                 "p.create_time, p.estimated_complete_time, p.is_completed "
                 "FROM projects p JOIN users u ON p.manager_id = u.id "
                 "WHERE p.id = ?");
    query.addBindValue(projectId);
    
    if(query.exec() && query.next())
    {
        project.id = query.value(0).toInt();
        project.name = query.value(1).toString();
        project.description = query.value(2).toString();
        project.managerId = query.value(3).toInt();
        project.managerName = query.value(4).toString();
        project.createTime = query.value(5).toDateTime();
        project.estimatedCompleteTime = query.value(6).toDateTime();
        project.isCompleted = query.value(7).toBool();
    }
    else
    {
        qDebug() << "Failed to get project by id: " << query.lastError().text();
    }
    
    return project;
}

int DataBaseManagement::AddProject(const Project& project)
{
    QSqlQuery query;
    query.prepare("INSERT INTO projects (name, description, manager_id, estimated_complete_time, is_completed) "
                 "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(project.name);
    query.addBindValue(project.description);
    query.addBindValue(project.managerId);
    query.addBindValue(project.estimatedCompleteTime);
    query.addBindValue(project.isCompleted);
    
    if(!query.exec())
    {
        qDebug() << "Failed to add project: " << query.lastError().text();
        return -1;
    }
    
    // 获取新插入项目的ID
    int projectId = query.lastInsertId().toInt();
    
    // 将项目经理添加为项目成员
    if(projectId > 0) {
        QVector<int> managerIdVec;
        managerIdVec.append(project.managerId);
        AssignUsersToProject(projectId, managerIdVec);
    }
    
    return projectId;
}

bool DataBaseManagement::UpdateProject(const Project& project)
{
    QSqlQuery query;
    query.prepare("UPDATE projects SET name = ?, description = ?, manager_id = ?, "
                 "estimated_complete_time = ?, is_completed = ? "
                 "WHERE id = ?");
    query.addBindValue(project.name);
    query.addBindValue(project.description);
    query.addBindValue(project.managerId);
    query.addBindValue(project.estimatedCompleteTime);
    query.addBindValue(project.isCompleted);
    query.addBindValue(project.id);
    
    if(!query.exec())
    {
        qDebug() << "Failed to update project: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::DeleteProject(int projectId)
{
    QSqlQuery query(_db);
    query.prepare("DELETE FROM projects WHERE id = ?");
    query.addBindValue(projectId);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "删除项目失败: " << query.lastError().text();
        return false;
    }
}

bool DataBaseManagement::AddProjectNode(const ProjectNode& node)
{
    QSqlQuery query;
    query.prepare("INSERT INTO project_nodes (project_id, name, description, parent_id, "
                 "estimated_completion_time, is_completed) "
                 "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(node.projectId);
    query.addBindValue(node.name);
    query.addBindValue(node.description);
    query.addBindValue(node.parentId > 0 ? node.parentId : QVariant());
    query.addBindValue(node.estimatedCompletionTime);
    query.addBindValue(node.isCompleted);
    
    if(!query.exec())
    {
        qDebug() << "Failed to add project node: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::UpdateProjectNode(const ProjectNode& node)
{
    QSqlQuery query;
    query.prepare("UPDATE project_nodes SET name = ?, description = ?, parent_id = ?, "
                 "estimated_completion_time = ?, is_completed = ? "
                 "WHERE id = ?");
    query.addBindValue(node.name);
    query.addBindValue(node.description);
    query.addBindValue(node.parentId > 0 ? node.parentId : QVariant());
    query.addBindValue(node.estimatedCompletionTime);
    query.addBindValue(node.isCompleted);
    query.addBindValue(node.id);
    
    if(!query.exec())
    {
        qDebug() << "Failed to update project node: " << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DataBaseManagement::DeleteProjectNode(int nodeId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM project_nodes WHERE id = ?");
    query.addBindValue(nodeId);
    
    if(!query.exec())
    {
        qDebug() << "Failed to delete project node: " << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

// 获取项目成员
QVector<User> DataBaseManagement::GetProjectUsers(int projectId)
{
    QVector<User> users;
    QSqlQuery query(_db);

    // 联合查询获取项目成员信息
    query.prepare("SELECT u.id, u.username, u.password, u.role, u.created_at "
                 "FROM users u "
                 "INNER JOIN project_user pu ON u.id = pu.user_id "
                 "WHERE pu.project_id = ?");
    query.addBindValue(projectId);

    if (query.exec()) {
        while (query.next()) {
            User user;
            user.id = query.value(0).toInt();
            user.userName = query.value(1).toString();
            user.password = query.value(2).toString();
            user.role = static_cast<UserRole>(query.value(3).toInt());
            user.createTime = query.value(4).toDateTime();
            users.append(user);
        }
    } else {
        qDebug() << "获取项目成员失败: " << query.lastError().text();
    }

    return users;
}

// 获取项目文件
QVector<FileInfo> DataBaseManagement::GetProjectFiles(int projectId, FileStatus status)
{
    QVector<FileInfo> files;
    QSqlQuery query(_db);

    // 使用project_file关联表查询
    query.prepare("SELECT f.id, f.file_name, f.file_path, f.file_extension, f.file_size, f.uploader_id, "
                 "u.username, f.upload_time, f.file_type, f.status, pf.project_id, f.is_process_document "
                 "FROM files f "
                 "JOIN users u ON f.uploader_id = u.id "
                 "JOIN project_file pf ON f.id = pf.file_id "
                 "WHERE pf.project_id = ? AND f.status = ?");
    query.addBindValue(projectId);
    query.addBindValue(static_cast<int>(status));
    
    if(query.exec())
    {
        while(query.next())
        {
            FileInfo file;
            file.id = query.value(0).toInt();
            file.fileName = query.value(1).toString();
            file.filePath = query.value(2).toString();
            file.fileExtension = query.value(3).toString();
            file.fileSize = query.value(4).toLongLong();
            file.uploaderId = query.value(5).toInt();
            file.uploaderName = query.value(6).toString();
            file.uploadTime = query.value(7).toDateTime();
            file.fileType = static_cast<FileType>(query.value(8).toInt());
            file.status = static_cast<FileStatus>(query.value(9).toInt());
            file.projectId = query.value(10).toInt();
            file.isProcessDocument = query.value(11).toBool();
            files.append(file);
        }
    }
    else
    {
        qDebug() << "获取项目文件失败: " << query.lastError().text();
    }
    
    return files;
}

// 分配用户到项目
bool DataBaseManagement::AssignUsersToProject(int projectId, const QVector<int>& userIds)
{
    // 开始事务
    _db.transaction();
    QSqlQuery query(_db);
    bool success = true;

    for (int userId : userIds) {
        query.prepare("INSERT OR IGNORE INTO project_user (project_id, user_id) VALUES (?, ?)");
        query.addBindValue(projectId);
        query.addBindValue(userId);

        if (!query.exec()) {
            qDebug() << "分配用户到项目失败: " << query.lastError().text();
            success = false;
            break;
        }
    }

    // 根据操作结果提交或回滚事务
    if (success) {
        _db.commit();
    } else {
        _db.rollback();
    }

    return success;
}

// 更新项目成员
bool DataBaseManagement::UpdateProjectUsers(int projectId, const QVector<int>& userIds)
{
    // 开始事务
    _db.transaction();
    QSqlQuery query(_db);
    bool success = true;

    // 1. 先删除项目的所有现有成员关联
    query.prepare("DELETE FROM project_user WHERE project_id = ?");
    query.addBindValue(projectId);

    if (!query.exec()) {
        qDebug() << "删除项目成员关联失败: " << query.lastError().text();
        _db.rollback();
        return false;
    }

    // 2. 添加新的成员关联
    for (int userId : userIds) {
        query.prepare("INSERT INTO project_user (project_id, user_id) VALUES (?, ?)");
        query.addBindValue(projectId);
        query.addBindValue(userId);

        if (!query.exec()) {
            qDebug() << "更新项目成员失败: " << query.lastError().text();
            success = false;
            break;
        }
    }

    // 根据操作结果提交或回滚事务
    if (success) {
        _db.commit();
    } else {
        _db.rollback();
    }

    return success;
}

// 分配文件到项目
bool DataBaseManagement::AssignFilesToProject(int projectId, const QVector<int>& fileIds)
{
    // 开始事务
    _db.transaction();
    QSqlQuery query(_db);
    bool success = true;

    for (int fileId : fileIds) {
        query.prepare("INSERT OR IGNORE INTO project_file (project_id, file_id) VALUES (?, ?)");
        query.addBindValue(projectId);
        query.addBindValue(fileId);

        if (!query.exec()) {
            qDebug() << "分配文件到项目失败: " << query.lastError().text();
            success = false;
            break;
        }
    }

    // 根据操作结果提交或回滚事务
    if (success) {
        _db.commit();
    } else {
        _db.rollback();
    }

    return success;
}

// 更新项目文件关联
bool DataBaseManagement::UpdateProjectFiles(int projectId, const QVector<int>& fileIds)
{
    // 开始事务
    _db.transaction();
    QSqlQuery query(_db);
    bool success = true;

    // 1. 先删除项目的所有现有文件关联
    query.prepare("DELETE FROM project_file WHERE project_id = ?");
    query.addBindValue(projectId);

    if (!query.exec()) {
        qDebug() << "清除项目文件关联失败: " << query.lastError().text();
        _db.rollback();
        return false;
    }

    // 2. 添加新的文件关联
    for (int fileId : fileIds) {
        query.prepare("INSERT INTO project_file (project_id, file_id) VALUES (?, ?)");
        query.addBindValue(projectId);
        query.addBindValue(fileId);

        if (!query.exec()) {
            qDebug() << "更新文件关联失败: " << query.lastError().text();
            success = false;
            break;
        }
    }

    // 根据操作结果提交或回滚事务
    if (success) {
        _db.commit();
        qDebug() << "成功更新项目" << projectId << "的文件关联，共" << fileIds.size() << "个文件";
    } else {
        _db.rollback();
    }

    return success;
}

bool DataBaseManagement::AssignFilesToNode(int nodeId, const QVector<int>& fileIds)
{
    // 首先删除该节点已有的关联关系
    QSqlQuery query(_db);
    query.prepare("DELETE FROM node_file WHERE node_id = ?");
    query.addBindValue(nodeId);
    
    if(!query.exec()) {
        qDebug() << "Failed to delete existing node-file relationships: " << query.lastError().text();
        return false;
    }
    
    // 添加新的关联关系
    for(int fileId : fileIds) {
        query.prepare("INSERT INTO node_file (node_id, file_id) VALUES (?, ?)");
        query.addBindValue(nodeId);
        query.addBindValue(fileId);
        
        if(!query.exec()) {
            qDebug() << "Failed to assign file to node: " << query.lastError().text();
            return false;
        }
    }
    
    return true;
}

QVector<FileInfo> DataBaseManagement::GetNodeFiles(int nodeId, FileStatus status)
{
    QVector<FileInfo> files;
    QSqlQuery query(_db);
    
    // 联合查询获取节点关联的文件信息
    query.prepare("SELECT f.id, f.file_name, f.file_path, f.uploader_id, u.username, f.upload_time, f.file_type, f.status "
                 "FROM files f "
                 "INNER JOIN node_file nf ON f.id = nf.file_id "
                 "LEFT JOIN users u ON f.uploader_id = u.id "
                 "WHERE nf.node_id = ? AND f.status = ?");
    query.addBindValue(nodeId);
    query.addBindValue(static_cast<int>(status));
    
    if(query.exec()) {
        while(query.next()) {
            FileInfo file;
            file.id = query.value(0).toInt();
            file.fileName = query.value(1).toString();
            file.filePath = query.value(2).toString();
            file.uploaderId = query.value(3).toInt();
            file.uploaderName = query.value(4).toString();
            file.uploadTime = query.value(5).toDateTime();
            file.fileType = static_cast<FileType>(query.value(6).toInt());
            file.status = static_cast<FileStatus>(query.value(7).toInt());
            files.append(file);
        }
    } else {
        qDebug() << "Failed to get node files: " << query.lastError().text();
    }
    
    return files;
}
