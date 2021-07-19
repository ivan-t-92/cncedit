QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../3rd-party/lexertl14/include \
     ../3rd-party/parsertl14/include \
     ../3rd-party/glm-0.9.9.8

TARGET = cncedit

SOURCES += \
    backplotwidget.cpp \
    boundingbox.cpp \
    codeeditor.cpp \
    controller.cpp \
    documentview.cpp \
    expr.cpp \
    geometry.cpp \
    highlighter.cpp \
    main.cpp \
    mainwindow.cpp \
    ncprogramblock.cpp \
    orthographicviewwidget.cpp \
    parser.cpp \
    s840d_alarm.cpp \
    value.cpp \
    variables.cpp

HEADERS += \
    backplotwidget.h \
    boundingbox.h \
    codeeditor.h \
    controller.h \
    documentview.h \
    expr.h \
    geometry.h \
    ggroupenum.h \
    highlighter.h \
    mainwindow.h \
    motion.h \
    ncprogramblock.h \
    orthographicviewwidget.h \
    parser.h \
    s840d_alarm.h \
    s840d_def.h \
    scopedtimer.h \
    util.h \
    value.h \
    variables.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

