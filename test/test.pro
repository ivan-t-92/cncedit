QT += testlib gui

CONFIG += qt console warn_on depend_includepath testcase c++17
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    tst_test_case_1.cpp \
    ../src/controller.cpp \
    ../src/s840d_alarm.cpp \
    ../src/parser.cpp \
    ../src/expr.cpp \
    ../src/value.cpp \
    ../src/variables.cpp \
    ../src/ncprogramblock.cpp \
    ../src/geometry.cpp


INCLUDEPATH += ../3rd-party/lexertl14/include \
     ../3rd-party/parsertl14/include \
     ../3rd-party/glm-0.9.9.8 \
     ../src
