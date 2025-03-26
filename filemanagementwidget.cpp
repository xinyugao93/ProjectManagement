#include <Python.h>
#include "filemanagementwidget.h"
#include "Databasemanagement.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QTextDocument>
#include <QProgressDialog>
#include <QDebug>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QTemporaryDir>
#include <QApplication>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>
#include <QThread>
#include <vector>
#include <string>

FileManagementWidget::FileManagementWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
    // 初始化Python解释器
    Py_Initialize();
}

FileManagementWidget::~FileManagementWidget()
{
    if(Py_IsInitialized())
    {
        Py_Finalize();
        qDebug() << "成功终止python解释器";
    }
}

void FileManagementWidget::setCurrentUser(const User &user)
{
    _currentUser = user;
    updateUIBasedOnRole();
    loadFileData(); // 重新加载文件数据
}

void FileManagementWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 创建堆叠页面
    _stackedWidget = new QStackedWidget(this);
    
    // 设置各个视图
    setupFileListView();
    setupProcessDocumentsView();
    setupRecycleBinView();
    
    mainLayout->addWidget(_stackedWidget);
    
    // 默认显示文件列表视图
    _stackedWidget->setCurrentIndex(0);
}

void FileManagementWidget::setupFileListView()
{
    _fileListView = new QWidget(_stackedWidget);
    QVBoxLayout* layout = new QVBoxLayout(_fileListView);
    
    // 顶部工具栏
    QHBoxLayout* toolLayout = new QHBoxLayout();
    
    // 文件类型筛选
    QLabel* filterLabel = new QLabel("文件类型:", _fileListView);
    _fileTypeFilter = new QComboBox(_fileListView);
    _fileTypeFilter->addItem("全部文件", -1);
    
    connect(_fileTypeFilter, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &FileManagementWidget::onRefreshFiles);
    
    toolLayout->addWidget(filterLabel);
    toolLayout->addWidget(_fileTypeFilter);
    
    // 搜索框
    _searchBox = new QLineEdit(_fileListView);
    _searchBox->setPlaceholderText("搜索文件...");
    connect(_searchBox, &QLineEdit::returnPressed, this, &FileManagementWidget::onSearchFile);
    toolLayout->addWidget(_searchBox);
    
    // 操作按钮
    QPushButton* processDocButton = new QPushButton("过程文档", _fileListView);
    QPushButton* recycleBinButton = new QPushButton("回收站", _fileListView);
    _uploadButton = new QPushButton("上传文件", _fileListView);
    _downloadButton = new QPushButton("下载文件", _fileListView);
    _deleteButton = new QPushButton("删除文件", _fileListView);
    
    connect(processDocButton, &QPushButton::clicked, this, &FileManagementWidget::onProcessDocument);
    connect(recycleBinButton, &QPushButton::clicked, this, &FileManagementWidget::onRecycleBin);
    connect(_uploadButton, &QPushButton::clicked, this, &FileManagementWidget::onUploadFile);
    connect(_downloadButton, &QPushButton::clicked, this, &FileManagementWidget::onDownloadFile);
    connect(_deleteButton, &QPushButton::clicked, this, &FileManagementWidget::onDeleteFile);
    
    toolLayout->addStretch();
    toolLayout->addWidget(processDocButton);
    toolLayout->addWidget(recycleBinButton);
    toolLayout->addWidget(_uploadButton);
    toolLayout->addWidget(_downloadButton);
    toolLayout->addWidget(_deleteButton);
    
    layout->addLayout(toolLayout);
    
    // 文件表格
    _filesTable = new QTableWidget(_fileListView);
    _filesTable->setColumnCount(6);
    _filesTable->setHorizontalHeaderLabels({"ID", "文件名", "类型", "大小", "上传者", "上传时间"});
    _filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _filesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _filesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _filesTable->setAlternatingRowColors(true);
    
    layout->addWidget(_filesTable);
    
    _stackedWidget->addWidget(_fileListView);
}

void FileManagementWidget::setupProcessDocumentsView()
{
    _processDocView = new QWidget(_stackedWidget);
    QVBoxLayout* layout = new QVBoxLayout(_processDocView);
    
    // 顶部工具栏
    QHBoxLayout* toolLayout = new QHBoxLayout();
    
    QPushButton* backButton = new QPushButton("返回文件列表", _processDocView);
    _printDocButton = new QPushButton("打印文档", _processDocView);
    _organizeDocButton = new QPushButton("整理文档", _processDocView);
    
    connect(backButton, &QPushButton::clicked, [this]() {
        _stackedWidget->setCurrentIndex(0);
        loadFileData();
    });
    
    // 打印文档按钮连接
    connect(_printDocButton, &QPushButton::clicked, [this]() {
        // 打印选中的文档
        QList<QTableWidgetItem*> selectedItems = _docsTable->selectedItems();
        if(selectedItems.count() == 0) {
            QMessageBox::warning(this, "提示", "请先选择要打印的文档");
            return;
        }
        
        // 获取选中的行
        QSet<int> selectedRows;
        for(const QTableWidgetItem* item : selectedItems) {
            selectedRows.insert(item->row());
        }
        
        if(selectedRows.size() == 0) {
            QMessageBox::warning(this, "提示", "请先选择要打印的文档");
            return;
        }
        
        // 只允许选择一个文档进行打印
        if(selectedRows.size() > 1) {
            QMessageBox::warning(this, "提示", "一次只能打印一个文档，请只选择一个文档");
            return;
        }
        
        int row = *selectedRows.begin(); // 获取选中的行
        int docId = _docsTable->item(row, 0)->text().toInt();
        QString docName = _docsTable->item(row, 1)->text();
        
        // 获取所有文件数据
        QVector<FileInfo> allFiles = DataBaseManagement::Instance()->GetAllFiles();
        
        // 查找文件信息
        FileInfo fileInfo;
        bool found = false;
        
        for(const FileInfo& file : allFiles) {
            if(file.id == docId) {
                fileInfo = file;
                found = true;
                break;
            }
        }
        
        if(!found) {
            QMessageBox::warning(this, "错误", "无法找到选中的文档");
            return;
        }
        
        // 检查文件是否存在
        QFileInfo qFileInfo(fileInfo.filePath);
        if(!qFileInfo.exists()) {
            QMessageBox::warning(this, "错误", "文件不存在或已被移动");
            return;
        }
        
        // 创建打印设置对话框
        QPrinter printer;
        QPrintDialog printDialog(&printer, this);
        if(printDialog.exec() != QDialog::Accepted) {
            return;
        }
        
        // 根据文档类型处理
        QString ext = qFileInfo.suffix().toLower();
        QFile file(fileInfo.filePath);
        
        if(file.open(QIODevice::ReadOnly)) {
            // 读取文件内容
            QByteArray fileData = file.readAll();
            file.close();
            
            // 对于Word文档，我们需要使用系统调用来打印
            // 临时创建要打印的文件的副本
            QString tempFilePath = QDir::tempPath() + "/" + qFileInfo.fileName();
            QFile tempFile(tempFilePath);
            if(tempFile.open(QIODevice::WriteOnly)) {
                tempFile.write(fileData);
                tempFile.close();
                
                // 使用ShellExecute打印
                QProcess::startDetached("rundll32.exe", QStringList() << "shell32.dll,ShellExec_RunDLL" << "print" << QDir::toNativeSeparators(tempFilePath));
                QMessageBox::information(this, "打印文档", "已将文档发送到打印队列");
            } else {
                QMessageBox::warning(this, "错误", "无法创建临时文件用于打印");
            }

        } else {
            QMessageBox::warning(this, "错误", "无法读取文件内容");
        }
    });
    
    // 整理文档按钮连接
    connect(_organizeDocButton, &QPushButton::clicked, [this]() {
        organizeDocuments();
    });
    
    toolLayout->addWidget(backButton);
    toolLayout->addStretch();
    toolLayout->addWidget(_organizeDocButton);
    toolLayout->addWidget(_printDocButton);
    
    layout->addLayout(toolLayout);
    
    // 文档表格
    _docsTable = new QTableWidget(_processDocView);
    _docsTable->setColumnCount(5);
    _docsTable->setHorizontalHeaderLabels({"ID", "文档名", "类型", "上传者", "上传时间"});
    _docsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _docsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _docsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _docsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _docsTable->setAlternatingRowColors(true);
    
    layout->addWidget(_docsTable);
    
    _stackedWidget->addWidget(_processDocView);
}

void FileManagementWidget::setupRecycleBinView()
{
    _recycleBinView = new QWidget(_stackedWidget);
    QVBoxLayout* layout = new QVBoxLayout(_recycleBinView);
    
    // 顶部工具栏
    QHBoxLayout* toolLayout = new QHBoxLayout();
    
    QPushButton* backButton = new QPushButton("返回文件列表", _recycleBinView);
    _restoreButton = new QPushButton("恢复文件", _recycleBinView);
    _permanentDeleteButton = new QPushButton("永久删除", _recycleBinView);
    
    connect(backButton, &QPushButton::clicked, [this]() {
        _stackedWidget->setCurrentIndex(0);
        loadFileData();
    });
    connect(_restoreButton, &QPushButton::clicked, [this]() {
        // 恢复选中的文件
        QList<QTableWidgetSelectionRange> ranges = _deletedFilesTable->selectedRanges();
        if(ranges.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择要恢复的文件");
            return;
        }

        int successCount = 0;
        int failCount = 0;
        
        // 遍历所有选中的行
        QSet<int> processedRows;
        for(const QTableWidgetSelectionRange& range : ranges) {
            for(int row = range.topRow(); row <= range.bottomRow(); ++row) {
                if(processedRows.contains(row)) {
                    continue; // 跳过已处理的行
                }
                processedRows.insert(row);
                
                int fileId = _deletedFilesTable->item(row, 0)->text().toInt();
                if(DataBaseManagement::Instance()->RestoreFile(fileId)) {
                    successCount++;
                } else {
                    failCount++;
                }
            }
        }
        
        if(successCount > 0) {
            QMessageBox::information(this, "成功", QString("已成功恢复%1个文件").arg(successCount));
            if(failCount > 0) {
                QMessageBox::warning(this, "部分失败", QString("有%1个文件恢复失败").arg(failCount));
            }
            loadFileData();
        } else if(failCount > 0) {
            QMessageBox::warning(this, "错误", "所有文件恢复均失败");
        }
    });
    connect(_permanentDeleteButton, &QPushButton::clicked, [this]() {
        // 永久删除选中的文件
        QList<QTableWidgetSelectionRange> ranges = _deletedFilesTable->selectedRanges();
        if(ranges.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择要删除的文件");
            return;
        }
        
        if(QMessageBox::question(this, "确认删除", QString("确定要永久删除选中的%1个文件吗？此操作不可恢复！").arg(ranges.size()),
                                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        
        int successCount = 0;
        int failCount = 0;
        
        // 遍历所有选中的行
        QSet<int> processedRows;
        for(const QTableWidgetSelectionRange& range : ranges) {
            for(int row = range.topRow(); row <= range.bottomRow(); ++row) {
                if(processedRows.contains(row)) {
                    continue; // 跳过已处理的行
                }
                processedRows.insert(row);
                
                int fileId = _deletedFilesTable->item(row, 0)->text().toInt();
                if(DataBaseManagement::Instance()->DeleteFile(fileId, true)) {
                    successCount++;
                } else {
                    failCount++;
                }
            }
        }
        
        if(successCount > 0) {
            QMessageBox::information(this, "成功", QString("已成功删除%1个文件").arg(successCount));
            if(failCount > 0) {
                QMessageBox::warning(this, "部分失败", QString("有%1个文件删除失败").arg(failCount));
            }
            loadFileData();
        } else if(failCount > 0) {
            QMessageBox::warning(this, "错误", "所有文件删除均失败");
        }
    });
    
    toolLayout->addWidget(backButton);
    toolLayout->addStretch();
    toolLayout->addWidget(_restoreButton);
    toolLayout->addWidget(_permanentDeleteButton);
    
    layout->addLayout(toolLayout);
    
    // 已删除文件表格
    _deletedFilesTable = new QTableWidget(_recycleBinView);
    _deletedFilesTable->setColumnCount(5);
    _deletedFilesTable->setHorizontalHeaderLabels({"ID", "文件名", "类型", "上传者", "删除时间"});
    _deletedFilesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _deletedFilesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _deletedFilesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _deletedFilesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _deletedFilesTable->setAlternatingRowColors(true);
    
    layout->addWidget(_deletedFilesTable);
    
    _stackedWidget->addWidget(_recycleBinView);
}

void FileManagementWidget::loadFileData()
{
    // 根据当前视图加载不同的数据
    int currentIndex = _stackedWidget->currentIndex();
    
    if(currentIndex == 0) {
        // 文件列表视图
        _filesTable->setRowCount(0);
        
        // 获取所有文件
        QVector<FileInfo> files = DataBaseManagement::Instance()->GetAllFiles();
        
        // 应用筛选
        int typeFilter = _fileTypeFilter->currentData().toInt();
        QString searchText = _searchBox->text().trimmed().toLower();
        
        for(const FileInfo& file : files) {
            // 应用类型筛选
            if(typeFilter != -1 && static_cast<int>(file.fileType) != typeFilter)
                continue;
            
            // 应用搜索筛选
            if(!searchText.isEmpty() && !file.fileName.toLower().contains(searchText))
                continue;
            
            int row = _filesTable->rowCount();
            _filesTable->insertRow(row);
            
            _filesTable->setItem(row, 0, new QTableWidgetItem(QString::number(file.id)));
            
            // 移除文件名中的后缀
            QString displayName = file.fileName;
            int dotPos = displayName.lastIndexOf('.');
            if(dotPos > 0) {
                displayName = displayName.left(dotPos);
            }
            _filesTable->setItem(row, 1, new QTableWidgetItem(displayName));
            
            // 文件类型
            _filesTable->setItem(row, 2, new QTableWidgetItem("文档"));
            
            // 文件大小
            QString sizeText;
            if(file.fileSize < 1024)
                sizeText = QString("%1 B").arg(file.fileSize);
            else if(file.fileSize < 1024 * 1024)
                sizeText = QString("%1 KB").arg(file.fileSize / 1024.0, 0, 'f', 2);
            else
                sizeText = QString("%1 MB").arg(file.fileSize / (1024.0 * 1024.0), 0, 'f', 2);
            _filesTable->setItem(row, 3, new QTableWidgetItem(sizeText));
            
            // 上传者
            _filesTable->setItem(row, 4, new QTableWidgetItem(file.uploaderName));
            
            // 上传时间
            _filesTable->setItem(row, 5, new QTableWidgetItem(file.uploadTime.toString("yyyy-MM-dd hh:mm:ss")));
            
            // 所属项目
            _filesTable->setItem(row, 6, new QTableWidgetItem(file.projectId > 0 ? QString::number(file.projectId) : "-"));
        }
    }
    else if(currentIndex == 1) {
        // 过程文档视图
        _docsTable->setRowCount(0);
        
        // 获取所有过程文档
        QVector<FileInfo> docs = DataBaseManagement::Instance()->GetProcessDocuments();
        
        for(const FileInfo& doc : docs) {
            int row = _docsTable->rowCount();
            _docsTable->insertRow(row);
            
            _docsTable->setItem(row, 0, new QTableWidgetItem(QString::number(doc.id)));
            
            // 移除文件名中的后缀
            QString displayName = doc.fileName;
            int dotPos = displayName.lastIndexOf('.');
            if(dotPos > 0) {
                displayName = displayName.left(dotPos);
            }
            _docsTable->setItem(row, 1, new QTableWidgetItem(displayName));
            
            // 文件类型
            _docsTable->setItem(row, 2, new QTableWidgetItem("文档"));
            
            // 上传者
            _docsTable->setItem(row, 3, new QTableWidgetItem(doc.uploaderName));
            
            // 上传时间
            _docsTable->setItem(row, 4, new QTableWidgetItem(doc.uploadTime.toString("yyyy-MM-dd hh:mm:ss")));
        }
    }
    else if(currentIndex == 2) {
        // 回收站视图
        _deletedFilesTable->setRowCount(0);
        
        // 获取所有已删除文件
        QVector<FileInfo> deletedFiles = DataBaseManagement::Instance()->GetAllFiles(FileStatus::DELETED);
        
        for(const FileInfo& file : deletedFiles) {
            int row = _deletedFilesTable->rowCount();
            _deletedFilesTable->insertRow(row);
            
            _deletedFilesTable->setItem(row, 0, new QTableWidgetItem(QString::number(file.id)));
            
            // 移除文件名中的后缀
            QString displayName = file.fileName;
            int dotPos = displayName.lastIndexOf('.');
            if(dotPos > 0) {
                displayName = displayName.left(dotPos);
            }
            _deletedFilesTable->setItem(row, 1, new QTableWidgetItem(displayName));
            
            _deletedFilesTable->setItem(row, 2, new QTableWidgetItem("文档"));
            
            // 上传者
            _deletedFilesTable->setItem(row, 3, new QTableWidgetItem(file.uploaderName));
            
            // 删除时间 (实际上是上传时间，这里简化处理)
            _deletedFilesTable->setItem(row, 4, new QTableWidgetItem(file.uploadTime.toString("yyyy-MM-dd hh:mm:ss")));
        }
    }
}

void FileManagementWidget::updateUIBasedOnRole()
{
    // 根据用户角色设置权限
    bool canUpload = (_currentUser.role == UserRole::ADMINISTRATOR || _currentUser.role == UserRole::PROJECTMANAGER);
    bool canDelete = (_currentUser.role == UserRole::ADMINISTRATOR);
    
    _uploadButton->setEnabled(canUpload);
    _deleteButton->setEnabled(canDelete);
    _permanentDeleteButton->setEnabled(canDelete);
}

void FileManagementWidget::onUploadFile()
{
    // 选择文件，允许选择.doc和.docx文件
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "", "Word文档(*.doc *.docx)");
    if(filePath.isEmpty())
        return;
    
    QFileInfo fileInfo(filePath);
    
    // 文件类型固定为文档类型
    FileType fileType = FileType::DOCUMENT;
    QString ext = fileInfo.suffix().toLower();
    
    // 是否为过程文档
    bool isProcessDoc = (QMessageBox::question(this, "过程文档", "是否将该文件标记为过程文档？",
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
    
    // 获取不带后缀的文件名
    QString baseFileName = fileInfo.completeBaseName();
    
    // 创建文件记录
    FileInfo newFile;
    newFile.fileName = fileInfo.fileName(); // 保存原始文件名（包含后缀）用于文件系统操作
    newFile.filePath = filePath;
    newFile.fileExtension = ext;
    newFile.fileSize = fileInfo.size();
    newFile.uploaderId = _currentUser.id;
    newFile.uploaderName = _currentUser.userName;
    newFile.uploadTime = QDateTime::currentDateTime();
    newFile.fileType = fileType;
    newFile.status = FileStatus::NORMAL;
    newFile.projectId = -1; // 暂不关联项目
    newFile.isProcessDocument = isProcessDoc;
    
    // 保存文件
    if(DataBaseManagement::Instance()->AddFile(newFile)) {
        QMessageBox::information(this, "成功", "文件上传成功");
        loadFileData(); // 刷新表格
    } else {
        QMessageBox::warning(this, "错误", "文件上传失败");
    }
}

void FileManagementWidget::onDownloadFile()
{
    // 获取选中的文件
    QList<QTableWidgetSelectionRange> ranges = _filesTable->selectedRanges();
    if(ranges.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要下载的文件");
        return;
    }
    
    // 只支持单个文件下载，如果选择了多个，提示用户一次只能下载一个文件
    if(ranges.size() > 1 || ranges[0].rowCount() > 1) {
        QMessageBox::warning(this, "提示", "一次只能下载一个文件，请只选择一个文件");
        return;
    }
    
    int row = ranges[0].topRow();
    
    // 获取文件ID
    int fileId = _filesTable->item(row, 0)->text().toInt();
    
    // 获取所有文件
    QVector<FileInfo> allFiles = DataBaseManagement::Instance()->GetAllFiles();
    
    // 查找选中的文件
    FileInfo selectedFile;
    bool found = false;
    
    for(const FileInfo& file : allFiles) {
        if(file.id == fileId) {
            selectedFile = file;
            found = true;
            break;
        }
    }
    
    if(!found) {
        QMessageBox::warning(this, "错误", "无法找到选中的文件");
        return;
    }
    
    // 让用户选择保存位置
    QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件", 
                                                      selectedFile.fileName,
                                                      QString("Word文档(*.%1)").arg(selectedFile.fileExtension));
    if(saveFilePath.isEmpty()) {
        return; // 用户取消了操作
    }
    
    // 复制文件到目标位置
    if(QFile::copy(selectedFile.filePath, saveFilePath)) {
        QMessageBox::information(this, "成功", "文件下载成功");
    } else {
        QMessageBox::warning(this, "错误", "文件下载失败，可能是源文件不存在或没有足够的权限");
    }
}

void FileManagementWidget::onProcessDocument()
{
    // 切换到过程文档视图
    _stackedWidget->setCurrentIndex(1);
    loadFileData();
}

void FileManagementWidget::onRecycleBin()
{
    // 切换到回收站视图
    _stackedWidget->setCurrentIndex(2);
    loadFileData();
}

void FileManagementWidget::onChangeView(int index)
{
    _stackedWidget->setCurrentIndex(index);
    loadFileData();
}

void FileManagementWidget::onRefreshFiles()
{
    loadFileData();
}

void FileManagementWidget::onDeleteFile()
{
    // 获取选中的文件
    QList<QTableWidgetSelectionRange> ranges = _filesTable->selectedRanges();
    if(ranges.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要删除的文件");
        return;
    }
    
    // 计算选中的不重复行数量
    QSet<int> selectedRows;
    for(const QTableWidgetSelectionRange& range : ranges) {
        for(int row = range.topRow(); row <= range.bottomRow(); ++row) {
            selectedRows.insert(row);
        }
    }
    
    // 确认删除
    if(QMessageBox::question(this, "确认删除", QString("确定要将选中的%1个文件移到回收站吗？").arg(selectedRows.size()),
                          QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    // 删除所有选中的文件
    for(int row : selectedRows) {
        int fileId = _filesTable->item(row, 0)->text().toInt();
        if(DataBaseManagement::Instance()->DeleteFile(fileId)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    if(successCount > 0) {
        QMessageBox::information(this, "成功", QString("已成功移动%1个文件到回收站").arg(successCount));
        if(failCount > 0) {
            QMessageBox::warning(this, "部分失败", QString("有%1个文件移动失败").arg(failCount));
        }
        loadFileData(); // 刷新表格
    } else if(failCount > 0) {
        QMessageBox::warning(this, "错误", "所有文件移动均失败");
    }
}

void FileManagementWidget::onSearchFile()
{
    loadFileData();
}

int FileManagementWidget::getWordDocumentPageCount(const QString& filePath)
{
    int pageCount = 1; // 默认至少1页
    
    // 使用文件大小估算页数
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        // 每500KB估算为1页，最小1页
        pageCount = qMax(1, static_cast<int>(fileInfo.size() / (500 * 1024)) + 1);
        qDebug() << "文档" << fileInfo.fileName() << "估算页数：" << pageCount;
    }
    
    return pageCount;
}

// 实现文档整理功能
void FileManagementWidget::organizeDocuments()
{
    // 获取选中的文档
    QList<QTableWidgetItem*> selectedItems = _docsTable->selectedItems();
    if(selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要整理的文档");
        return;
    }
    
    // 获取选中的行
    QSet<int> selectedRows;
    for(const QTableWidgetItem* item : selectedItems) {
        selectedRows.insert(item->row());
    }
    
    if(selectedRows.empty()) {
        QMessageBox::warning(this, "提示", "请先选择要整理的文档");
        return;
    }
    
    // 获取所有过程文档
    QVector<FileInfo> allDocs = DataBaseManagement::Instance()->GetProcessDocuments();
    
    // 收集选中的文档信息
    QVector<FileInfo> selectedDocs;
    for(int row : selectedRows) {
        int docId = _docsTable->item(row, 0)->text().toInt();
        
        // 查找文档信息
        for(const FileInfo& doc : allDocs) {
            if(doc.id == docId) {
                selectedDocs.append(doc);
                break;
            }
        }
    }
    
    if(selectedDocs.isEmpty()) {
        QMessageBox::warning(this, "提示", "未能获取选中文档的信息");
        return;
    }
    
    // 确认整理操作
    if(QMessageBox::question(this, "确认整理", 
                           QString("确定要合并选中的 %1 个文档并添加目录吗？").arg(selectedDocs.size()),
                           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    
    // 合并文档并添加目录
    mergeDocumentsByPython(selectedDocs);
}

// 合并文档并添加目录
void FileManagementWidget::mergeDocumentsByPython(const QVector<FileInfo>& documents)
{
    if(documents.empty()) {
        QMessageBox::warning(this, "错误", "没有可合并的文档");
        return;
    }

    // 让用户选择保存位置
    QString saveFilePath = QFileDialog::getSaveFileName(
        this,
        "保存合并后的文档",
        QDir::homePath() + "/合并文档.docx",
        "Word文档 (*.docx);;所有文件 (*.*)");

    if(saveFilePath.isEmpty()) {
        // 用户取消了保存对话框
        return;
    }

    // 确保文件扩展名为.docx
    if(!saveFilePath.endsWith(".docx", Qt::CaseInsensitive)) {
        saveFilePath += ".docx";
    }

    // 获取当前应用程序路径
    QString appDirPath = QCoreApplication::applicationDirPath();

    // 构建Python脚本路径 - 更新为实际位置
    QString pythonScriptDir = appDirPath + "/main.py";

    QVector<QString> combineFiles;
    for(auto &file : documents)
    {
        QString inputFile = file.filePath;
        qDebug() << "合并文件路径：" << inputFile;
        combineFiles.emplace_back(inputFile);
    }

    if(!Py_IsInitialized())
    {
        QMessageBox::warning(this, "错误", "无法初始化python解释器");
        return;
    }

    try
    {
        PyObject* pModule = PyImport_ImportModule("main");
        if(!pModule)
        {
            QMessageBox::warning(this, "错误", "无法导入python模块");
            PyErr_Print();
            Py_Finalize();
            return;
        }

        // 获取函数对象
        PyObject* pFunc = PyObject_GetAttrString(pModule, "merge_documents");
        if(!pFunc || !PyCallable_Check(pFunc))
        {
            QMessageBox::warning(this, "错误", "无法找到函数‘merge_documents’");
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            Py_Finalize();
            return;
        }

        PyObject* pInputFiles = PyList_New(combineFiles.size());
        int index = 0;
        for(auto &file : combineFiles)
        {
            PyObject* pFile = PyUnicode_FromString(file.toStdString().c_str());
            PyList_SetItem(pInputFiles, index++, pFile);
        }

        PyObject* pOutputFile = PyUnicode_FromString(saveFilePath.toStdString().c_str());

        PyObject* pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, pInputFiles);
        PyTuple_SetItem(pArgs, 1, pOutputFile);

        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        if(!pResult)
        {
            QMessageBox::warning(this, "错误", "python函数调用失败");
        }

        Py_XDECREF(pResult);
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
    }
    catch(const std::exception &e)
    {
        QMessageBox::warning(this, "错误", "发生异常：%1", e.what());
    }
    catch(...)
    {
        QMessageBox::warning(this, "错误", "发生未知异常");
    }

    QMessageBox::information(this, "提醒", "文档合并成功，打开word文档后，选择更新域以手动更新目录");
}

void FileManagementWidget::mergeDocuments(const QVector<FileInfo>& documents)
{
    if(documents.empty()) {
        QMessageBox::warning(this, "错误", "没有可合并的文档");
        return;
    }
    
    // 让用户选择保存位置
    QString saveFilePath = QFileDialog::getSaveFileName(
        this, 
        "保存合并后的文档", 
        QDir::homePath() + "/合并文档.docx",
        "Word文档 (*.docx);;所有文件 (*.*)");
    
    if(saveFilePath.isEmpty()) {
        // 用户取消了保存对话框
        return;
    }
    
    // 确保文件扩展名为.docx
    if(!saveFilePath.endsWith(".docx", Qt::CaseInsensitive)) {
        saveFilePath += ".docx";
    }
    
    // 创建进度对话框，并确保始终显示
    QProgressDialog progress("正在合并文档...", "取消", 0, documents.count() + 3, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    progress.show();
    QCoreApplication::processEvents();
    
    // 尝试手动终止可能已经存在的Word进程
    progress.setLabelText("正在检查Word进程...");
    QCoreApplication::processEvents();
    
    #ifdef Q_OS_WIN
    // 在Windows上尝试查找并结束可能未正常关闭的Word进程
    QProcess tasklist;
    tasklist.start("tasklist", QStringList() << "/FI" << "IMAGENAME eq WINWORD.EXE" << "/FO" << "CSV");
    if(tasklist.waitForFinished(5000)) {
        QString output = QString::fromLocal8Bit(tasklist.readAllStandardOutput());
        if(output.contains("WINWORD.EXE")) {
            QMessageBox::information(this, "提示", "检测到Word程序正在运行。\n请先关闭所有Word文档，然后点击确定继续。");
        }
    }
    #endif
    
    // 创建Word应用程序
    QAxObject* wordApp = nullptr;
    QAxObject* wordDocs = nullptr;
    QAxObject* wordDoc = nullptr;
    
    // 添加超时控制
    QTimer timer;
    timer.setSingleShot(true);
    bool operationTimedOut = false;
    
    QEventLoop loop;
    connect(&timer, &QTimer::timeout, [&]() {
        operationTimedOut = true;
        loop.quit();
    });
    
    try {
        // 设置超时时间（15秒）- 减少超时时间以便更快响应
        const int TIMEOUT_MS = 15000;
        
        // 创建Word应用程序
        progress.setLabelText("正在启动Word应用程序...");
        progress.setValue(1);
        QCoreApplication::processEvents();
        
        // 尝试多次创建Word应用实例
        const int MAX_ATTEMPTS = 3;
        for(int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
            try {
                timer.start(TIMEOUT_MS);
                wordApp = new QAxObject("Word.Application", this);
                timer.stop();
                
                if(operationTimedOut) {
                    delete wordApp;
                    wordApp = nullptr;
                    
                    if(attempt < MAX_ATTEMPTS) {
                        progress.setLabelText(QString("正在尝试重新启动Word (尝试 %1/%2)...").arg(attempt + 1).arg(MAX_ATTEMPTS));
                        QCoreApplication::processEvents();
                        QThread::sleep(2); // 等待2秒再尝试
                        continue;
                    } else {
                        throw QString("启动Word应用程序超时，请确保Microsoft Word已正确安装并且没有被其他程序占用");
                    }
                }
                
                if(!wordApp || wordApp->isNull()) {
                    delete wordApp;
                    wordApp = nullptr;
                    
                    if(attempt < MAX_ATTEMPTS) {
                        progress.setLabelText(QString("Word启动失败，正在重试 (尝试 %1/%2)...").arg(attempt + 1).arg(MAX_ATTEMPTS));
                        QCoreApplication::processEvents();
                        QThread::sleep(2); // 等待2秒再尝试
                        continue;
                    } else {
                        throw QString("无法启动Word应用程序，请确保已安装Microsoft Word");
                    }
                }
                
                // 成功创建Word应用
                break;
            } catch(...) {
                if(wordApp) {
                    delete wordApp;
                    wordApp = nullptr;
                }
                
                if(attempt >= MAX_ATTEMPTS) {
                    throw QString("尝试启动Word失败，请检查您的Microsoft Office安装");
                }   
                
                progress.setLabelText(QString("Word初始化失败，正在重试 (尝试 %1/%2)...").arg(attempt + 1).arg(MAX_ATTEMPTS));
                QCoreApplication::processEvents();
                QThread::sleep(2); // 等待2秒再尝试
            }
        }
        
        // 设置Word应用程序属性
        try {
            progress.setLabelText("正在配置Word应用程序...");
            progress.setValue(2);
            QCoreApplication::processEvents();
            
            wordApp->setProperty("Visible", QVariant(false));
            wordApp->setProperty("DisplayAlerts", QVariant(false)); // 禁用警告弹窗
        } catch(...) {
            throw QString("配置Word应用程序失败，可能是Office安装不完整");
        }
        
        // 获取文档集合
        progress.setLabelText("正在创建文档...");
        QCoreApplication::processEvents();
        
        timer.start(TIMEOUT_MS);
        wordDocs = wordApp->querySubObject("Documents");
        timer.stop();
        
        if(operationTimedOut || !wordDocs || wordDocs->isNull()) {
            throw QString("无法获取Word文档集合");
        }
        
        // 创建新文档
        timer.start(TIMEOUT_MS);
        wordDoc = wordDocs->querySubObject("Add()");
        timer.stop();
        
        if(operationTimedOut) {
            throw QString("创建Word文档超时，可能是Word没有正确响应");
        }
        
        if(!wordDoc || wordDoc->isNull()) {
            throw QString("无法创建新Word文档");
        }
        
        // 使用更简单的方式合并文档，采用文本复制方式而不是复杂的COM交互
        progress.setLabelText("正在准备合并内容...");
        progress.setValue(3);
        QCoreApplication::processEvents();
        
        // 创建一个简单的文档内容
        QString mergedContent = "<html><body>";
        mergedContent += "<h1 style='text-align:center'>文档目录</h1>";
        mergedContent += "<div style='page-break-after:always'></div>";
        
        // 添加文档内容
        progress.setLabelText("正在合并文档内容...");
        QCoreApplication::processEvents();
        
        bool anyDocumentsProcessed = false;
        
        for(int i = 0; i < documents.size(); i++) {
            progress.setValue(i + 4); // 3 + 1 因为之前已经设置到3了
            if(progress.wasCanceled()) {
                throw QString("用户取消操作");
            }
            
            const FileInfo& doc = documents[i];
            QCoreApplication::processEvents();
            
            // 构建可能的文件路径
            QStringList pathsToTry;
            pathsToTry << doc.filePath;
            pathsToTry << doc.filePath + "/" + doc.fileName;
            pathsToTry << doc.fileName;
            
            QString filePath;
            bool fileFound = false;
            
            for(const QString& path : pathsToTry) {
                QFileInfo fileInfo(path);
                if(fileInfo.exists() && fileInfo.isFile()) {
                    filePath = path;
                    fileFound = true;
                    break;
                }
            }
            
            if(!fileFound) {
                QMessageBox::warning(this, "警告", QString("文件 '%1' 不存在，将跳过").arg(doc.fileName));
                continue;
            }
            
            // 添加文档标题
            QString docTitle = doc.fileName;
            int dotPos = docTitle.lastIndexOf('.');
            if(dotPos > 0) {
                docTitle = docTitle.left(dotPos);
            }
            
            mergedContent += QString("<h2>%1</h2>").arg(docTitle);
            
            // 添加文档内容的引用
            mergedContent += QString("<p>文件: %1</p>").arg(filePath);
            mergedContent += "<p>请参阅原始文档了解详细内容</p>";
            
            // 添加分页符
            if(i < documents.size() - 1) {
                mergedContent += "<div style='page-break-after:always'></div>";
            }
            
            anyDocumentsProcessed = true;
        }
        
        mergedContent += "</body></html>";
        
        // 如果没有处理任何文档，抛出异常
        if(!anyDocumentsProcessed) {
            throw QString("没有找到任何有效的文档可以合并");
        }
        
        // 使用QTextDocument来设置内容
        QTextDocument textDoc;
        textDoc.setHtml(mergedContent);
        
        // 将内容写入Word文档
        timer.start(TIMEOUT_MS);
        try {
            // 获取Word文档的内容范围
            QAxObject* content = wordDoc->querySubObject("Content");
            if(content && !content->isNull()) {
                // 插入文本内容
                content->dynamicCall("InsertAfter(QString)", textDoc.toHtml());
                delete content;
            } else {
                throw QString("无法获取Word文档内容");
            }
        } catch(...) {
            throw QString("将内容写入Word文档时出错");
        }
        timer.stop();
        
        if(operationTimedOut) {
            throw QString("写入文档内容超时");
        }
        
        // 将文档保存到用户指定的位置
        progress.setLabelText("正在保存文档...");
        progress.setValue(documents.size() + 2);
        QCoreApplication::processEvents();
        
        QString nativeSavePath = QDir::toNativeSeparators(saveFilePath);
        
        timer.start(TIMEOUT_MS);
        try {
            // 使用SaveAs保存文档
            wordDoc->dynamicCall("SaveAs(const QString&)", nativeSavePath);
        } catch(...) {
            throw QString("保存文档时出错");
        }
        timer.stop();
        
        if(operationTimedOut) {
            throw QString("保存文档超时，可能是文件太大或磁盘空间不足");
        }
        
        // 关闭Word应用程序
        progress.setLabelText("正在关闭Word...");
        progress.setValue(documents.size() + 3);
        QCoreApplication::processEvents();
        
        // 关闭Word文档
        try {
            if(wordDoc && !wordDoc->isNull()) {
                wordDoc->dynamicCall("Close(bool)", false);
            }
        } catch(...) {}
        
        // 退出Word应用
        try {
            if(wordApp && !wordApp->isNull()) {
                wordApp->dynamicCall("Quit()");
            }
        } catch(...) {}
        
        // 释放COM对象
        if(wordDoc) delete wordDoc;
        if(wordDocs) delete wordDocs;
        if(wordApp) delete wordApp;
        
        wordDoc = nullptr;
        wordDocs = nullptr;
        wordApp = nullptr;
        
        // 完成合并，隐藏进度对话框
        progress.close();
        
        // 询问用户是否打开合并后的文档
        if(QMessageBox::question(this, "合并完成", 
                                "文档已成功合并并保存到:\n" + saveFilePath + "\n\n是否立即打开该文档?", 
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(saveFilePath));
        }
        
    } catch(const QString& error) {
        progress.close();
        QMessageBox::critical(this, "错误", QString("合并文档时出错: %1").arg(error));
        
        // 尝试清理：关闭Word
        try {
            if(wordDoc && !wordDoc->isNull()) {
                wordDoc->dynamicCall("Close(bool)", false);
            }
        } catch(...) {}
        
        try {
            if(wordApp && !wordApp->isNull()) {
                wordApp->dynamicCall("Quit()");
            }
        } catch(...) {}
        
        // 强制终止WINWORD进程 (仅适用于Windows)
        #ifdef Q_OS_WIN
        QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "WINWORD.EXE");
        #endif
        
        // 释放资源
        if(wordDoc) delete wordDoc;
        if(wordDocs) delete wordDocs;
        if(wordApp) delete wordApp;
    }
}

void FileManagementWidget::insertTableOfContents(QAxObject* wordDocument)
{
    if(!wordDocument || wordDocument->isNull()) {
        return;
    }
    
    try {
        // 获取Selection对象
        QAxObject* app = wordDocument->querySubObject("Application");
        if(!app || app->isNull()) {
            return;
        }
        
        QAxObject* selection = app->querySubObject("Selection");
        if(!selection || selection->isNull()) {
            delete app;
            return;
        }
        
        // 设置标题样式
        selection->dynamicCall("HomeKey(int)", QVariant(6)); // wdStory = 6
        
        // 获取当前选区范围
        QAxObject* range = selection->querySubObject("Range");
        if(!range || range->isNull()) {
            delete app;
            delete selection;
            throw QString("无法获取Range对象");
        }
        
        // 添加字段代码来创建目录
        // 使用 TOC 字段代码创建目录，可以指定级别范围为1-3
        QString fieldCode = "TOC \\o \"1-3\" \\h \\z \\u";
        QAxObject* fields = range->querySubObject("Fields");
        if (!fields || fields->isNull()) {
            delete app;
            delete selection;
            delete range;
            throw QString("无法获取Fields对象");
        }
        
        QVariant paramRange(range->asVariant());
        QVariant paramCode(fieldCode);
        fields->dynamicCall("Add(IDispatch*, int, const QString&, bool)", 
                            paramRange, QVariant(1), paramCode, QVariant(true));
        
        // 在目录后添加两个空行
        selection->dynamicCall("EndKey(int)", QVariant(6)); // 移动到文档末尾
        selection->dynamicCall("TypeParagraph()");
        selection->dynamicCall("TypeParagraph()");
        
        // 释放对象
        delete fields;
        delete range;
        delete selection;
        delete app;
        
    } catch(const QString& error) {
        QMessageBox::warning(nullptr, "警告", QString("插入目录时出错: %1").arg(error));
    }
}
