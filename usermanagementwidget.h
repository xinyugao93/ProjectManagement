#ifndef USERMANAGEMENTWIDGET_H
#define USERMANAGEMENTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include "DBModels.h"

class UserManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UserManagementWidget(QWidget *parent = nullptr);
    ~UserManagementWidget();

    void setCurrentUser(const User& user);

private slots:
    void onAddUser();
    void onEditUser();
    void onDeleteUser();
    void onRefreshTable();
    void onRoleChanged(int index);

private:
    void setupUI();
    void loadUserData();
    void updateUIBasedOnRole();

private:
    User _currentUser;
    QTableWidget* _usersTable;
    QPushButton* _addButton;
    QPushButton* _editButton;
    QPushButton* _deleteButton;
    QPushButton* _refreshButton;
    QComboBox* _roleFilter;
    QLineEdit* _searchBox;
};

#endif // USERMANAGEMENTWIDGET_H 