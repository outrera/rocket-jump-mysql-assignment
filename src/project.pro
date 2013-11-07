QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

#don't create bundle on OSX
CONFIG-=app_bundle

ROOT = $${PWD}/..
DESTDIR      = $${ROOT}

TARGET       = outbin

SOURCES = \
    main.cpp

HEADERS += \
    common.h

#RESOURCES += resources.qrc

OTHER_FILES += \
    task.txt

#remove the annoyances
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall
#QMAKE_CXXFLAGS += -save-temps

# GCC < 4.7
#QMAKE_CXXFLAGS += "-std=c++0x"

# GCC >= 4.7
#QMAKE_CXXFLAGS += -std=c++11

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/mysql/release/ -lmysqlclient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/mysql/debug/ -lmysqlclient
else:unix: LIBS += -L$$PWD/../lib/mysql/ -lmysqlclient

INCLUDEPATH += $$PWD/../lib/mysql/include
DEPENDPATH += $$PWD/../lib/mysql/include

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../lib/mysql/release/mysqlclient.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../lib/mysql/debug/mysqlclient.lib
else:unix: PRE_TARGETDEPS += $$PWD/../lib/mysql/libmysqlclient.a

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/mysql/release/ -lmysqlclient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/mysql/debug/ -lmysqlclient
else:unix: LIBS += -L$$PWD/../lib/mysql/ -lmysqlclient

INCLUDEPATH += $$PWD/../lib/mysql/include
DEPENDPATH += $$PWD/../lib/mysql/include
