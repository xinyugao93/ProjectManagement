#ifndef FILEMANAGEMENTWIDGET_H
#define FILEMANAGEMENTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QStackedWidget>
#include <QAxObject>
#include <QProgressDialog>
#include "DBModels.h"

class FileManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FileManagementWidget(QWidget *parent = nullptr);
    ~FileManagementWidget();

    void setCurrentUser(const User& user);

private slots:
    void onUploadFile();
    void onDownloadFile();
    void onProcessDocument();
    void onRecycleBin();
    void onChangeView(int index);
    void onRefreshFiles();
    void onDeleteFile();
    void onSearchFile();

private:
    void setupUI();
    void setupFileListView();
    void setupProcessDocumentsView();
    void setupRecycleBinView();
    void loadFileData();
    void updateUIBasedOnRole();
    int getWordDocumentPageCount(const QString& filePath);
    void organizeDocuments();
    void mergeDocuments(const QVector<FileInfo>& documents);
    void insertTableOfContents(QAxObject* wordDocument);
    
    // 创建简单的合并文档（用于没有安装Office的情况）
    void createSimpleMergedDocument(const QVector<FileInfo>& documents, const QString& saveFilePath, QProgressDialog& progress);

private:
    User _currentUser;
    QStackedWidget* _stackedWidget;
    
    // 文件列表视图
    QWidget* _fileListView;
    QTableWidget* _filesTable;
    QPushButton* _uploadButton;
    QPushButton* _downloadButton;
    QPushButton* _deleteButton;
    QLineEdit* _searchBox;
    QComboBox* _fileTypeFilter;
    
    // 过程文档视图
    QWidget* _processDocView;
    QTableWidget* _docsTable;
    QPushButton* _printDocButton;
    QPushButton* _organizeDocButton;
    
    // 回收站视图
    QWidget* _recycleBinView;
    QTableWidget* _deletedFilesTable;
    QPushButton* _restoreButton;
    QPushButton* _permanentDeleteButton;
};

#endif // FILEMANAGEMENTWIDGET_H
