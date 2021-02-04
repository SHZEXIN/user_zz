#-------------------------------------------------
#
# Project created by QtCreator 2021-02-03T13:09:22
#
#-------------------------------------------------
#DEFINES += MSVC #编译器套件，如果使用MINGW就注释掉此行
QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = decoder_demo1
TEMPLATE = app
CONFIG+= console
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        mpeg2ts.cpp \
        mysocket.cpp

HEADERS += \
        definitionsofmpeg2.h \
        ffmpegheader.h \
        mainwindow.h \
        mpeg2ts.h \
        mysocket.h

FORMS += \
        mainwindow.ui
contains(DEFINES,MSVC){
message('-->>(DEFINES,MSVC)')

##############ffmpeg----MSVC
INCLUDEPATH += $$PWD/../../ffmpeg-win/include \
            D:/SDL2-devel-2.0.14-VC/SDL2-2.0.14/include

###################链接动态库
# -L表示路径  -l（小写的L）表示动态库名
LIBS += $$PWD/../../ffmpeg-win/bin/avcodec.lib\
        $$PWD/../../ffmpeg-win/bin/avdevice.lib\
        $$PWD/../../ffmpeg-win/bin/avfilter.lib\
        $$PWD/../../ffmpeg-win/bin/avformat.lib\
        $$PWD/../../ffmpeg-win/bin/avutil.lib\
        $$PWD/../../ffmpeg-win/bin/swresample.lib\
        $$PWD/../../ffmpeg-win/bin/swscale.lib
LIBS += -L$$PWD/../../ffmpeg-win/bin -lavcodec -lavdevice -lavformat -lswresample -lswscale -lavfilter
LIBS += -lWs2_32
##############SDL2
LIBS += -LD:/SDL2-devel-2.0.14-VC/SDL2-2.0.14/lib/x64/SDL2
LIBS += -LD:/SDL2-devel-2.0.14-VC/SDL2-2.0.14/lib/x64 -lSDL2
 INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.10240.0/ucrt"
 LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.10240.0/ucrt/x64"
}
else{
message('-->>(DEFINES,MINGW) nothing')
##############ffmpeg----MINGW
INCLUDEPATH += $$PWD/../../ffmpeg-win/include
LIBS += $$PWD/../../ffmpeg-win/bin/avcodec.lib\
        $$PWD/../../ffmpeg-win/bin/avdevice.lib\
        $$PWD/../../ffmpeg-win/bin/avfilter.lib\
        $$PWD/../../ffmpeg-win/bin/avformat.lib\
        $$PWD/../../ffmpeg-win/bin/avutil.lib\
        $$PWD/../../ffmpeg-win/bin/swresample.lib\
        $$PWD/../../ffmpeg-win/bin/swscale.lib
###################链接动态库
# -L表示路径  -l（小写的L）表示动态库名----MINGW
LIBS += -L$$PWD/../../ffmpeg-win/bin -lavcodec-58 -lavdevice-58 -lavformat-58  -lswresample-3 -lswscale-5 -lavfilter-7
LIBS += -lpthread libwsock32 libws2_32
##############SDL2
INCLUDEPATH += $$PWD/../../SDL2-devel-2.0.14-mingw/SDL2-2.0.14/x86_64-w64-mingw32/include/SDL2
LIBS += -LD:/SDL2-devel-2.0.14-mingw/SDL2-2.0.14/x86_64-w64-mingw32/bin -lSDL2
}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
