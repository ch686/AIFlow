#include "exporter.h"
#include "rectshape.h"
#include <QMessageBox>
#include <QSqlError>

Exporter::Exporter(const QString &dbpath)
{
    //_scene=scene;
    _db= QSqlDatabase::addDatabase("QSQLITE", "conn1");
    _db.setDatabaseName(dbpath);
    bool ret=_db.open();
    if(ret)
    {
        _query=new QSqlQuery(_db);

        createTable();
        //get_class_names();
    }
    else
    {
        // 拼接错误信息
        QString info = QString("open失败详情\n"
                               "驱动类型: %1\n"
                               "数据库路径: %2\n"
                               "错误文本: %3\n"
                               "错误类型: %4")
                .arg(_db.driverName())
                .arg(_db.databaseName())
                .arg(_db.lastError().text())
                .arg((int)_db.lastError().type());

        // 弹出提示框
        QMessageBox::critical(nullptr, "数据库打开失败", info);
        _query=nullptr;
    }

}

Exporter::~Exporter()
{
    if(_query)
    {
        delete _query;
        _query=nullptr;
    }
    _db.close();
}

QColor Exporter::getClassMapColor(int classId)
{
    int mod = classId % 32;
    // 步长13 和32互质，0~32遍历不重复、色相跳跃拉开差距
    qreal hueStep = 13.0 * (360.0 / 32.0);
    qreal hue = fmod(mod * hueStep, 360.0);

    qreal sat = 0.72;
    qreal val = 0.38; // 深色基调
    return QColor::fromHsvF(hue / 360.0, sat, val);
}

int Exporter::DataCount()
{
    QString sql;
    sql="SELECT COUNT(id) AS total_count FROM image_data;";
    int ret =_query->exec(sql);
    if (!ret)
        return 0;
    if ( !_query->first())
    {
        return 0;
    }
    else
    {
        //_query->next();
        int val= _query->value("total_count").toInt();
        return val;
    }
}

const QStringList& Exporter::get_class_names()
{
    QString class_names=GetConfig("model_det", "class_names", "");
    if(class_names.isEmpty())
    {
        _class_names.clear();
        return _class_names;
    }
    _class_names=class_names.split(",");
    return _class_names;
}

QString Exporter::GetConfig(const QString& key, const QString& subkey, const QString& defval)
{
    QString val;
    QString sql;
    sql=QString::asprintf("select key_val from config_data where main_key='%s' and sub_key='%s'",
                          key.toUtf8().constData(), subkey.toUtf8().constData());
    int ret =_query->exec(sql);
    if (!ret)
        return val;
    if ( !_query->first())
    {
        sql=QString::asprintf("INSERT INTO config_data (main_key,sub_key,key_val) VALUES ('%s','%s','%s');",
                              key.toUtf8().constData(), subkey.toUtf8().constData(), defval.toUtf8().constData());
        ret =_query->exec(sql);
        val = defval;
    }
    else
    {
        //_query->next();
        val= _query->value("key_val").toString();
    }
    return val;
}

int Exporter::SetConfig(const QString& key, const QString& subkey, const QString& val)
{
    QString sql;
    sql=QString::asprintf("select * from config_data where main_key='%s' and sub_key='%s'",
                          key.toUtf8().constData(), subkey.toUtf8().constData());
    int ret =_query->exec(sql);
    if (!ret)
        return 0;
    if ( !_query->first())
    {
        sql=QString::asprintf("INSERT INTO config_data (main_key,sub_key,key_val) VALUES ('%s','%s','%s')",
                              key.toUtf8().constData(), subkey.toUtf8().constData(), val.toUtf8().constData());
        ret =_query->exec(sql);
    }
    else
    {
        sql=QString::asprintf("update config_data set key_val='%s' where main_key='%s' and sub_key='%s'",
                              val.toUtf8().constData(), key.toUtf8().constData(), subkey.toUtf8().constData());
        ret =_query->exec(sql);
    }
    return 1;
}

int Exporter::createTable()
{
    QString sql;
    int ret=0;

    sql = R"(
           CREATE TABLE IF NOT EXISTS config_data(
           id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
           main_key CHAR[128] NOT NULL,
           sub_key CHAR[128] NOT NULL,
           key_val TEXT DEFAULT (NULL))
           )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;

    sql = R"(
             CREATE TABLE IF NOT EXISTS image_data(
             id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
             img_name CHAR[128] NOT NULL,
             img_w INT DEFAULT (0),
             img_h INT DEFAULT (0),
             check_times INT DEFAULT (0))
           )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;

    sql = R"(
         CREATE TABLE IF NOT EXISTS rect_data(
         id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
         img_id int NOT NULL,
         class_id INT DEFAULT (0),
         x INT DEFAULT (0),
         y INT DEFAULT (0),
         w INT DEFAULT (0),
         h INT DEFAULT (0))
         )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;

    sql = R"(
         CREATE TABLE IF NOT EXISTS obb_data(
         id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
         img_id int NOT NULL,
         class_id INT DEFAULT (0),
         x1 INT DEFAULT (0),
         y1 INT DEFAULT (0),
         x2 INT DEFAULT (0),
         y2 INT DEFAULT (0),
         x3 INT DEFAULT (0),
         y3 INT DEFAULT (0),
         x4 INT DEFAULT (0),
         y4 INT DEFAULT (0))
         )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;
    //int visible;
    //int sub_type;
    sql = R"(
           CREATE TABLE IF NOT EXISTS polyn_data(
           id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
           img_id int NOT NULL,
           class_id INT DEFAULT (0),
           pt_type INT DEFAULT (0),
           pen_size INT DEFAULT (0),
           visible INT DEFAULT (0),
           sub_type INT DEFAULT (0))
           )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;

    sql = R"(
           CREATE TABLE IF NOT EXISTS polyn_points(
           id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
           img_id int NOT NULL,
           polyn_id int NOT NULL,
           x INT DEFAULT (0),
           y INT DEFAULT (0))
           )";
    ret =_query->exec(sql);
    if (!ret)
        return 0;

    return 1;

}

int Exporter::image_to_db(const QString &name,const QSize &img_size)
{
    if(!_query)
        return -1;
    QString sql;
    sql=QString::asprintf("SELECT id FROM image_data where img_name='%s'", name.toUtf8().constData());
    int ret =_query->exec(sql);
    if (!ret)
    {
       return -1;
    }
    int img_id = 0;
    if ( !_query->first())
    {
        sql =QString::asprintf("INSERT INTO image_data (img_name,img_w,img_h) VALUES ('%s',%d,%d);",
                                 name.toUtf8().constData(), img_size.width(), img_size.height());
        ret =_query->exec(sql);
        sql = "SELECT last_insert_rowid() as last_id;";
        ret =_query->exec(sql);
        _query->next();
        img_id=_query->value("last_id").toInt();
    }
    else
    {
        //_query->next();
        img_id= _query->value("id").toInt();
        sql =QString::asprintf("update image_data set img_w=%d,img_h=%d where id=%d",
                               img_size.width(), img_size.height(), img_id);
        ret =_query->exec(sql);

    }
    return img_id;
}

//=======load===========
int Exporter::db_to_labels(const QString &name,QList<RECT_INFO> &allInfo)
{
    if(!_query)
        return 0;
    QString sql;
    allInfo.clear();
    sql =QString::asprintf("SELECT id FROM image_data where img_name='%s'",name.toUtf8().constData());
    int ret =_query->exec(sql);
    if(!ret)
        return 0;
    int img_id=-1;
    if ( !_query->first())
    {
        return 0;
    }
    //_query->next();
    img_id=_query->value("id").toInt();

    sql=QString::asprintf("SELECT class_id,x,y,w,h FROM rect_data where img_id=%d",img_id);
    ret =_query->exec(sql);
    if ( !_query->first())
    {
        return 0;
    }
    do
    {
        RECT_INFO label;
        label.class_id=_query->value("class_id").toInt();
        int x=_query->value("x").toInt();
        int y=_query->value("y").toInt();
        int w=_query->value("w").toInt();
        int h=_query->value("h").toInt();
        label.rcBox.setRect(x,y,w,h);
        allInfo.push_back(label);
    }while (_query->next());
    return allInfo.size();
}

int Exporter::labels_to_shapes(const QList<RECT_INFO> &allInfo,Canvas *scene)
{
    QList<QGraphicsItem*> allItems = scene->items();
    foreach(QGraphicsItem * item, allItems)
    {
        if(item->type()==RectShape::Type
           //     ||item->type()==PolyShape::Type
          //     ||item->type()==PointShape::Type
           //     ||item->type()==RotatedRectShape::Type
          )
          _scene->removeItem(item);
    }

    for (auto& label : allInfo)
    {
        RectShape* shape = new RectShape(label.rcBox);
        int idx=label.class_id;
        if(idx>=0&&idx<_class_names.size())
        {
            shape->updateLabelText(_class_names[idx],idx);
            shape->updateLabelPosition(shape);
        }
        QColor bkcolor=getClassMapColor(idx);
        QColor bkcolor1=  bkcolor;
        QColor bkcolor2=  bkcolor;
        bkcolor1.setAlpha(50);
        bkcolor2.setAlpha(120);
        shape->setupStyle(((RectShape*)shape),bkcolor,
                                bkcolor1,
                                bkcolor2);
        scene->addItem(shape);
    }
    return 1;
}


int Exporter::load_shapes(Canvas *scene,const QString &name,const QStringList &class_names)
{
    _class_names=class_names;

    QList<RECT_INFO> allInfo;
    db_to_labels(name,allInfo);

    return labels_to_shapes(allInfo,scene);
}

//=======save===========
int Exporter::shapes_to_labels(Canvas *scene,QList<RECT_INFO> &allInfo)
{
    QList<QGraphicsItem*> allItems = scene->items();
    //QList<RECT_INFO> allInfo;
    allInfo.clear();
    foreach(QGraphicsItem * item, allItems)
    {
        bool is_temp=item->data(DATA_is_temp).toBool();
        if (item->type() == RectShape::Type&&!is_temp)
        {
            RectShape* rcItem= (RectShape*)item;
            QRectF rect = rcItem->rect();
            QPointF p1 = rcItem->mapToScene(rect.topLeft());
            QPointF p2 = rcItem->mapToScene(rect.bottomRight());
            QString text;
            int class_id=rcItem->getLabelText(text);

            allInfo.append(RECT_INFO{QRectF(p1,p2),class_id});
        }
    }
    return allInfo.size();
}


int Exporter::labels_to_db(int img_id,const QList<RECT_INFO> &allInfo)
{
    if(!_query)
        return 0;

    QString sql;
    sql =QString::asprintf("DELETE FROM rect_data WHERE img_id = %d",img_id);
    int ret =_query->exec(sql);
    if (!ret)
    {
       return 0;
    }

    for (auto& label : allInfo)
    {
        sql =QString::asprintf("INSERT INTO rect_data (img_id,class_id,x,y,w,h) VALUES (%d,%d,%d,%d,%d,%d);",
                               img_id,label.class_id,
                               qRound(label.rcBox.x()),
                               qRound(label.rcBox.y()),
                               qRound(label.rcBox.width()),
                               qRound(label.rcBox.height()));
        ret =_query->exec(sql);
    }
    return 1;
}

int Exporter::save_shapes(Canvas *scene,const QString &name,const QSize &img_size,const QStringList &class_names)
{
  //     shapes_data = []
    QList<RECT_INFO> allInfo;
    shapes_to_labels(scene,allInfo);
    int image_id=image_to_db(name,img_size);
    return labels_to_db(image_id,allInfo);
}
