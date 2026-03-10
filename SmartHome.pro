QT       += core gui network sql svg printsupport

# 头文件搜索路径
INCLUDEPATH += $$PWD \
               $$PWD/models \
               $$PWD/ui \
               $$PWD/network \
               $$PWD/database \
               $$PWD/services

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17

win32-g++:QMAKE_CXXFLAGS += -Wa,-mbig-obj

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    loginwidget.cpp \
    database/dao/AlarmDao.cpp \
    ui/alarmwidget.cpp \
    ui/devicecontrolwidget.cpp \
    ui/historywidget.cpp \
    ui/homewidget.cpp \
    ui/scenewidget.cpp \
    ui/settingswidget.cpp \
    services/historyservice.cpp \
    services/environmentservice.cpp \
    services/sceneservice.cpp \
    services/deviceservice.cpp \
    services/alarmservice.cpp \
    services/settingsservice.cpp \
    services/loginservice.cpp \
    network/networkmanager.cpp \
    database/databasemanager.cpp \
    database/dao/SystemConfigDao.cpp \
    database/dao/UserDao.cpp \
    database/dao/DeviceDao.cpp \
    database/dao/SceneDao.cpp \
    database/dao/EnvRecordDao.cpp \
    qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    loginwidget.h \
    models/User.h \
    database/dao/AlarmDao.h \
    ui/alarmwidget.h \
    ui/devicecontrolwidget.h \
    ui/historywidget.h \
    ui/homewidget.h \
    ui/scenewidget.h \
    ui/settingswidget.h \
    services/historyservice.h \
    services/environmentservice.h \
    services/sceneservice.h \
    services/deviceservice.h \
    services/alarmservice.h \
    services/settingsservice.h \
    services/loginservice.h \
    services/servicemodels.h \
    network/networkmanager.h \
    network/protocol.h \
    database/databasemanager.h \
    database/DatabaseConfig.h \
    database/dao/SystemConfigDao.h \
    database/dao/UserDao.h \
    database/dao/DeviceDao.h \
    database/dao/SceneDao.h \
    database/dao/EnvRecordDao.h \
    qcustomplot.h

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
