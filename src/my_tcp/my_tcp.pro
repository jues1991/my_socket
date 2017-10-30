TEMPLATE = lib
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


# version
VER_MAJ = 0
VER_MIN = 0
VER_PAT = 1



SOURCES += \
    my_tcp_client.cpp \
    my_tcp_server.cpp \
    my_tcp.cpp

HEADERS += \
    my_tcp_client.h \
    my_tcp_server.h \
    my_tcp_def.h \
    my_tcp.h



# boost
LIBS += -lboost_system -lboost_thread


#debug
#DEFINES += MY_SOCKET_DEBUG

# lib
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

