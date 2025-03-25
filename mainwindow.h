#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include "DBModels.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class UserManagementWidget;
class FileManagementWidget;
class ProjectManagementWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void SetCurrentUser(const User& user);

private slots:
    void onNavigationClicked(int index);
    void onLogout();

private:
    void setupUI();
    void setupNavigationPanel();
    void updateNavigationPanel();

private:
    Ui::MainWindow *ui;
    
    User _currentUser;
    QStackedWidget* _contentWidget;
    QWidget* _navigationPanel;
    
    UserManagementWidget* _userManagementWidget;
    FileManagementWidget* _fileManagementWidget;
    ProjectManagementWidget* _projectManagementWidget;
};
#endif // MAINWINDOW_H
