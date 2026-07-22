#include "DetDataset.h"
#include "net_utils/readfile.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <QSqlQuery>
#include <QString>
#include <QDir>
#include <QFileInfo>

std::vector<BBox> loadTXT(std::string txt_path)
{
	std::vector<BBox> objects;
	// 打开文件
	std::ifstream file(txt_path);
	if (!file.is_open())
	{
		// 文件打开失败直接返回空
		return objects;
	}

	std::string line;
	// 逐行读取
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		BBox box;
		// 按空格拆分读取 类别 cx cy w h
		iss >> box.class_id >> box.cx >> box.cy >> box.w >> box.h;
		objects.push_back(box);
	}

	file.close();
	return objects;
}

// 根据无后缀文件名，拼接目录寻找真实图片
static std::string findImageFullPath(const std::string& dir, const std::string& baseName)
{
    QDir qDir(QString::fromStdString(dir));
    // 支持的图片后缀
    const QStringList exts = {".jpg", ".png", ".bmp", ".JPG", ".PNG", ".BMP"};
    QString base = QString::fromStdString(baseName);

    for (const QString& ext : exts)
    {
        QString filePath = qDir.filePath(base + ext);
        if (QFileInfo::exists(filePath))
        {
            return filePath.toStdString();
        }
    }
    // 找不到任何匹配图片返回空字符串
    return "";
}

void load_det_data_from_folder(std::shared_ptr<Exporter> exporter,
                               const std::string &data_dir,
                               std::vector<TrainDataset> &train_datas,
                               std::vector<TrainDataset> &val_datas,                               
                               std::vector<int> ratio,
                               int batch_size,
                               bool use_same_data)
{
    QSqlQuery *query = exporter->_query;
    if (!query || ratio.size() != 2 || ratio[0] <= 0 || ratio[1] <= 0)
        return;

    // 清空输出容器
    train_datas.clear();
    val_datas.clear();

    // 1. 数据库读取所有图片，存入总临时列表
    std::map<std::string, TrainDataset> temp_map;
    const QString sql = R"(
        SELECT
            i.img_name, i.img_w, i.img_h,
            r.class_id, r.x, r.y, r.w, r.h
        FROM image_data i
        LEFT JOIN rect_data r ON i.id = r.img_id
        ORDER BY i.img_name
    )";
    if (!query->exec(sql))
        return;

    while (query->next())
    {
        QString q_img_name = query->value("img_name").toString();
        std::string img_base = q_img_name.toStdString();
        int imgW = query->value("img_w").toInt();
        int imgH = query->value("img_h").toInt();

        std::string full_path = findImageFullPath(data_dir, img_base);
        if (full_path.empty()) // 跳过不存在的图片
            continue;

        bool hasRect = !query->isNull("class_id");
        TrainDataset &ds = temp_map[img_base];
        if (ds.image_path.empty())
            ds.image_path = full_path;

        if (!hasRect || imgW <= 0 || imgH <= 0)
            continue;

        int clsId = query->value("class_id").toInt();
        int px = query->value("x").toInt();
        int py = query->value("y").toInt();
        int pw = query->value("w").toInt();
        int ph = query->value("h").toInt();

        BBox box;
        box.class_id = clsId;
        box.cx = (px + pw / 2.0f) / static_cast<float>(imgW);
        box.cy = (py + ph / 2.0f) / static_cast<float>(imgH);
        box.w  = static_cast<float>(pw) / static_cast<float>(imgW);
        box.h  = static_cast<float>(ph) / static_cast<float>(imgH);
        ds.bboxes.push_back(box);
    }

    // map转总数据集
    std::vector<TrainDataset> all_datas;
    for (auto &item : temp_map)
        all_datas.push_back(std::move(item.second));
    if (all_datas.empty())
        return;

    // 2. 随机打乱
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::shuffle(all_datas.begin(), all_datas.end(), rng);

    // 3. 按比例划分 9:1
    int total_weight = ratio[0] + ratio[1];
    int train_split = static_cast<int>(all_datas.size() * (1.0 * ratio[0] / total_weight));

    // 前 train_split 训练集
    for (int i = 0; i < train_split; i++)
        train_datas.push_back(std::move(all_datas[i]));
    // 剩余验证集
    for (size_t i = train_split; i < all_datas.size(); i++)
        val_datas.push_back(std::move(all_datas[i]));
    if(use_same_data)
    {
        for(auto &it:val_datas)
            train_datas.push_back(it);
    }
}
