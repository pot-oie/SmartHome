QT       += core gui network sql svg

# 头文件搜索路径
INCLUDEPATH += $$PWD \
               $$PWD/ui \
               $$PWD/network \
               $$PWD/database

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    loginwidget.cpp \
    ui/alarmwidget.cpp \
    ui/devicecontrolwidget.cpp \
    ui/historywidget.cpp \
    ui/homewidget.cpp \
    ui/scenewidget.cpp \
    ui/settingswidget.cpp \
    network/networkmanager.cpp \
    database/databasemanager.cpp

HEADERS += \
    mainwindow.h \
    loginwidget.h \
    ui/alarmwidget.h \
    ui/devicecontrolwidget.h \
    ui/historywidget.h \
    ui/homewidget.h \
    ui/scenewidget.h \
    ui/settingswidget.h \
    network/networkmanager.h \
    network/protocol.h \
    database/databasemanager.h

FORMS += \
    mainwindow.ui \
    loginwidget.ui \
    ui/alarmwidget.ui \
    ui/devicecontrolwidget.ui \
    ui/historywidget.ui \
    ui/homewidget.ui \
    ui/scenewidget.ui \
    ui/settingswidget.ui \

RESOURCES += \
    resources/resources.qrc

TRANSLATIONS += \
    translations/SmartHome_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
