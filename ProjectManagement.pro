QT       += core gui sql widgets printsupport axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Databasemanagement.cpp \
    filemanagementwidget.cpp \
    logindialog.cpp \
    main.cpp \
    mainwindow.cpp \
    projectmanagementwidget.cpp \
    usermanagement.cpp \
    usermanagementwidget.cpp

HEADERS += \
    DBModels.h \
    Databasemanagement.h \
    filemanagementwidget.h \
    logindialog.h \
    mainwindow.h \
    projectmanagementwidget.h \
    usermanagement.h \
    usermanagementwidget.h

FORMS += \
    logindialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
