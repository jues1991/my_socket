TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


# version
VER_MAJ = 0
VER_MIN = 0
VER_PAT = 1



SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../my_tcp/release/ -lmy_tcp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../my_tcp/debug/ -lmy_tcp
else:unix: LIBS += -L$$OUT_PWD/../my_tcp/ -lmy_tcp

INCLUDEPATH += $$PWD/../my_tcp
DEPENDPATH += $$PWD/../my_tcp




# boost
LIBS += -lboost_system -lboost_thread -lboost_program_options
