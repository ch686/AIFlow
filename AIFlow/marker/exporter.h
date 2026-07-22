#ifndef EXPORTER_H
#define EXPORTER_H
#include "canvas.h"
#include <QSqlDatabase>
#include <QSqlQuery>

typedef struct tagImageInfo
{
    QString name;
    QSizeF img_size;
    QRectF rc_roi;
    int img_thresh;
}IMG_INFO;

typedef struct tagLabelInfo
{
    int id;
    QList<QPointF> polygon;
    int pt_type;
    int pen_size;
    int class_id;
    int visible;
    int sub_type;
}POLY_INFO;

typedef struct tagRectInfo
{
    QRectF rcBox;
    int class_id;
}RECT_INFO;

class Exporter
{
public:
    Exporter(const QString &dbpath);
    ~Exporter();
    static QColor getClassMapColor(int classId);

    const QStringList&  get_class_names();
    int SetConfig(const QString& key, const QString& subkey, const QString& val);
    QString GetConfig(const QString& key, const QString& subkey, const QString& defval);

    int DataCount();

    int shapes_to_labels(Canvas *scene,QList<RECT_INFO> &allInfo);
    int labels_to_db(int img_id,const QList<RECT_INFO> &allInfo);

    int db_to_labels(const QString &name,QList<RECT_INFO> &allInfo);
    int labels_to_shapes(const QList<RECT_INFO> &allInfo,Canvas *scene);

    int save_shapes(Canvas *scene,const QString &name,const QSize &img_size,const QStringList &class_names);
    int load_shapes(Canvas *scene,const QString &name,const QStringList &class_names);

    QSqlQuery *_query;
private:
    int image_to_db(const QString &name,const QSize &img_size);
    //int shape_to_db(int img_id,const QList<RECT_INFO> &allInfo);
    int db_to_shape(const QString &name, QList<RECT_INFO> &allInfo);
    int createTable();

    QStringList _class_names;
    Canvas *_scene=nullptr;
    QSqlDatabase _db;
};

#endif // EXPORTER_H
