#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QSettings>
#include <cmath>
#include <QTextCodec>
#include "dnn_yolo.h"


int main(int argc, char *argv[])
{
//    DnnYolo detr("best_hagridv2.onnx",34);

//    cv::Mat img=cv::imread("1.jpg");
//    std::vector<Detection> det_results;
//    detr.detect(img,det_results);

//    cv::rectangle(img,det_results[0].box,CV_RGB(255,255,0));
//      cv::imshow("img",img);
//      cv::waitKey(0);

//    return 1;


//    // ---- 1. 环境变量优先 ----
//         QByteArray envScale = qgetenv("QT_SCALE_FACTOR");
//         if (!envScale.isEmpty()) {
//             qDebug() << "scale from env:" << envScale;
//             goto run;
//         }

//         // ---- 2. 临时 QApplication 检测屏幕 ----
//         {
//             QApplication tmp(argc, argv);
//             QScreen* s = tmp.primaryScreen();

//             // 配置文件
// //            QSettings cfg(QSettings::IniFormat, QSettings::UserScope,
// //                          "YourCompany", "YourApp");
// //            qreal saved = cfg.value("Display/scale", 0).toDouble();
// //            if (saved > 0.5) {
// //                envScale = QByteArray::number(saved, 'f', 2);
// //                qDebug() << "scale from config:" << envScale;
// //                goto run;
// //            }

//             // 自动检测
//             {
//                 qreal mmW = s->physicalSize().width();
//                 qreal mmH = s->physicalSize().height();
//                 int   pxW = s->size().width();
//                 int   pxH = s->size().height();

//                 qreal ppi = std::sqrt(pxW*pxW + pxH*pxH)
//                           / (std::sqrt(mmW*mmW + mmH*mmH) / 25.4);

//                 qreal scale;
//                 //if      (ppi > 170) scale = 2.0;
//                 //if (ppi > 130) scale = 1.5;
//                 if (ppi > 105) scale = 1.25;
//                 else                 scale = 1.0;

//                 envScale = QByteArray::number(scale, 'f', 2);
// //                cfg.setValue("Display/scale", scale);
//                 qDebug() << "auto detected scale:" << scale << "(PPI:" << ppi << ")";
//             }
//         }

//     run:
//         qputenv("QT_SCALE_FACTOR", envScale);
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=1");
 #if defined(_MSC_VER)
         QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
 #endif
         QApplication app(argc, argv);
         app.setWindowIcon(QIcon(":/new/prefix1/res/QNodeTest.ico"));
//
         // 1. 读取样式文件
         QFile styleFile(":/new/prefix2/res/style.qss"); // 资源文件路径
         // QFile styleFile("./dark_theme.qss"); // 外部本地文件用这个

         QString styleSheet;
         if (styleFile.open(QFile::ReadOnly))
         {
             styleSheet = QLatin1String(styleFile.readAll());
             styleFile.close();
         }

         // 2. 全局设置应用样式，所有窗口生效
         app.setStyleSheet(styleSheet);
//         qDebug() << "final scale:" << envScale;

         MainWindow w;
         w.show();
         w.showMaximized();
         return app.exec();
}
