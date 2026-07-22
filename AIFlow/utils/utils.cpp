#include "utils.h"
#include <QTextCodec>
#include <fstream>

namespace util
{

float loadBestLoss(const std::string& loss_file)
{
    std::ifstream in(loss_file);
    float loss = -1;
    if(in.is_open())
    {
        in >> loss;
        in.close();
    }
    return loss;
}


QMainWindow* getTopMainWindow(QWidget* widget)
{
    QWidget* cur = widget;
    while (cur != nullptr)
    {
        // 判断当前控件是否是QMainWindow
        if (QMainWindow* mainWin = qobject_cast<QMainWindow*>(cur))
        {
            return mainWin;
        }
        cur = cur->parentWidget(); // 向上找父窗口
    }
    return nullptr; // 没找到
}

// GBK 原始 char* → std::string(UTF8)
QMainWindow *g_main_window=nullptr;

std::string gbk_to_utf8(const char* gbk_str)
{
    if (!gbk_str)
        return "";

    QTextCodec* codecGbk = QTextCodec::codecForName("GBK");
    QString unicode = codecGbk->toUnicode(gbk_str);
    QByteArray utf8Bytes = unicode.toUtf8();

    return std::string(utf8Bytes.constData(), utf8Bytes.size());
}

//std::string gbk_to_utf8(const QString & gbk_str)
//{

//    return gbk_to_utf8(gbk_str.toLatin1().constData());
//}

// UTF8 char* → std::string(GBK)
std::string utf8_to_gbk(const char* utf8_str)
{
    if (!utf8_str)
        return "";

    QString unicode = QString::fromUtf8(utf8_str);
    QTextCodec* codecGbk = QTextCodec::codecForName("GBK");
    QByteArray gbkBytes = codecGbk->fromUnicode(unicode);

    return std::string(gbkBytes.constData(), gbkBytes.size());
}
}
