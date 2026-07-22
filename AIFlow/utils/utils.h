#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <QString>
#include <QMainWindow>

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include <dwmapi.h>
    #pragma comment(lib, "dwmapi.lib")
    #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
        #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
    #endif
#endif

namespace util
{
float loadBestLoss(const std::string& loss_file);
QMainWindow* getTopMainWindow(QWidget* widget);
extern QMainWindow *g_main_window;
std::string gbk_to_utf8(const char* gbk_str);
//std::string gbk_to_utf8(const QString & gbk_str);
std::string utf8_to_gbk(const char* utf8_str);
}

#endif // UTILS_H
