QT += core gui
QT += sql
QT += printsupport
#QTPLUGIN += qpng qjpeg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += NODE_EDITOR_SHARED
DEFINES -= UNICODE
DEFINES += NOMINMAX

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    darktreestyle.cpp \
    marker/baseshape.cpp \
    marker/canvas.cpp \
    marker/canvasdlg.cpp \
    marker/canvasview.cpp \
    main.cpp \
    mainwindow.cpp \
    marker/exporter.cpp \
    marker/handleitem.cpp \
    marker/obbhandle.cpp \
    marker/pointshape.cpp \
    marker/polyshape.cpp \
    marker/rectshape.cpp \
    marker/rotatedrectshape.cpp \
    models/model_camera_reader.cpp \
    models/model_image_reader.cpp \
    models/model_image_show.cpp \
    models/model_net_predict.cpp \
    models/model_net_train.cpp \
    models/model_onnx_reader.cpp \
    trainer/DetDataset.cpp \
    trainer/Detector.cpp \
    trainer/net/CSPdarknet53_tiny.cpp \
    trainer/net/yolo4_tiny.cpp \
    trainer/net/yolo_training.cpp \
    trainer/net_utils/Augmentations.cpp \
    trainer/net_utils/netutil.cpp \
    trainer/qcustomplot.cpp \
    trainer/traindlg.cpp \
    utils/utils.cpp \
    yolo_detect.cpp
    dnn_yolo.cpp

HEADERS += \
    darktreestyle.h \
    marker/baseshape.h \
    marker/canvas.h \
    marker/canvasdlg.h \
    marker/canvasview.h \
    marker/exporter.h \
    marker/handleitem.h \
    marker/obbhandle.h \
    marker/pointshape.h \
    marker/polyshape.h \
    marker/rectshape.h \
    marker/rotatedrectshape.h \
    marker/shapeutil.h \
    models/model_camera_reader.h \
    models/model_image_reader.h \
    models/model_image_show.h \
    models/model_net_predict.h \
    models/model_net_train.h \
    models/model_onnx_reader.h \
    nodes/node_add_image.h \
    nodes/node_dataset.h \
    nodes/node_image.h \
    mainwindow.h \
    nodes/node_net.h \
    trainer/DetDataset.h \
    trainer/Detector.h \
    trainer/net/CSPdarknet53_tiny.h \
    trainer/net/yolo4_tiny.h \
    trainer/net/yolo_training.h \
    trainer/net_utils/Augmentations.h \
    trainer/net_utils/_dirent.h \
    trainer/net_utils/netutil.h \
    trainer/net_utils/readfile.h \
    trainer/qcustomplot.h \
    trainer/traindlg.h \
    utils/utils.h \
    yolo_detect.h
    dnn_yolo.h

FORMS += \
    mainwindow.ui

OPENCV_DIR = D:/env/opencv/opencv500/build/install

INCLUDEPATH += $$PWD/marker
INCLUDEPATH += $$PWD/utils
INCLUDEPATH += $$PWD/forms
INCLUDEPATH += $$PWD/models
INCLUDEPATH += $$PWD/nodes
INCLUDEPATH += $$PWD/trainer
INCLUDEPATH += $$OPENCV_DIR/include
INCLUDEPATH += D:/env/libtorch201cpu/libtorch/include
INCLUDEPATH += D:/env/libtorch201cpu/libtorch/include/torch/csrc/api/include

INCLUDEPATH += $$PWD/../QNodes/include
INCLUDEPATH += $$PWD/../QNodes/include/QtNodes/internal
# 末尾添加
LIBS += -luser32
CONFIG(release, debug|release){
    LIBS += -L$$PWD/../output/QNodes/release -lQNodes
} else {
    LIBS += -L$$PWD/../output/QNodes/debug -lQNodes
}
#DEFINES+=USE_CVDNN
#D:/env/opencv/opencv500/build/install/include
#CONFIG += console
CONFIG(release, debug|release) {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib \
        -lopencv_core500 \
        -lopencv_imgcodecs500 \
        -lopencv_imgproc500 \
        -lopencv_highgui500 \
        -lopencv_dnn500 \
        -lopencv_freetype500 \
        -lopencv_videoio500
    LIBS += -LD:/env/libtorch201cpu/libtorch/lib \
        -lc10 \
        -ltorch_cpu
} else {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib \
        -lopencv_core500d \
        -lopencv_imgcodecs500d \
        -lopencv_imgproc500d \
        -lopencv_highgui500d \
        -lopencv_dnn500d \
        -lopencv_freetype500d \
        -lopencv_videoio500d
    LIBS += -LD:/env/libtorch201cpu/libtorch_d/lib \
        -lc10 \
        -ltorch_cpu
}

msvc {
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS += /utf-8
    RC_FILE += VisionFlow.rc

}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
