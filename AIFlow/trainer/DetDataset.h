#pragma once
#include"net_utils/netutil.h"
#include<opencv2/opencv.hpp>
#include"net_utils/Augmentations.h"
#include "exporter.h"
#include "utils.h"


std::vector<BBox> loadTXT(std::string txt_path);

class DetDataset :public torch::data::Dataset<DetDataset> {
private:
	// Declare 2 std::vectors of tensors for images and labels
    std::vector<TrainDataset> _train_datas;
    bool _isTrain = true;
    int _width = 416;
    int _height = 416;
    float _hflipProb = 0;
    float _vflipProb = 0;
    float _noiseProb = 0;
    float _brightProb = 0;
    float _noiseMuLimit = 1;
    float _noiseSigmaLimit = 1;
    float _brightContrastLimit = 0.2;
    float _brightnessLimit = 0;
    std::map<std::string, float> _name_idx = {};
public:
	// Constructor
    DetDataset(std::vector<TrainDataset> &train_datas,std::vector<std::string> &class_names, bool istrain = true,
        int width = 416, int height = 416, float hflip_prob = 0.5, float vflip_prob = 0)
	{
        _train_datas = train_datas;
        _isTrain = istrain;
        _width = width;
        _height = height;
        _hflipProb = hflip_prob;
        _vflipProb = vflip_prob;
		for (int i = 0; i < class_names.size(); i++)
		{
            _name_idx.insert(std::pair<std::string, float>(class_names[i], float(i)));
		}
	};

	// Override get() function to return tensor at location index
    torch::data::Example<> get(size_t index) override
    {
        auto &data= _train_datas.at(index);
        std::string image_path = data.image_path;

        cv::Mat img = cv::imread(util::utf8_to_gbk(image_path.c_str()));
        if(img.empty())
        {
            std::ifstream fs(image_path, std::ios::binary);
            if (fs.is_open())
            {
                std::vector<char> buf((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
                fs.close();

                // 直接解码二进制数据，完全忽略文件名后缀
                cv::Mat img = cv::imdecode(cv::Mat(buf), cv::IMREAD_COLOR);
            }
        }
        if(img.empty())
        {
            CV_Assert("error");
        }

        std::vector<BBox> &boxes =data.bboxes;
        //Data m_data(img, boxes);
        cv::resize(img, img, cv::Size(_width, _height));

        //m_data = Augmentations::Resize(m_data, _width, _height, 1);
		//Augmentations can be implemented here...

        //float width_under1 = 1.0 / m_data.image.cols;//box的长宽归一化用
        //float height_under1 = 1.0 / m_data.image.rows;//box的长宽归一化用
        torch::Tensor img_tensor = torch::from_blob(img.data, {img.rows, img.cols, 3 }, torch::kByte).permute({ 2, 0, 1 }); // Channels x Height x Width

        int box_num = boxes.size();
        if (boxes.size() == 0)
		{
			torch::Tensor label_tensor = torch::ones({ 1 });
            //std::cout << annotation_path << std::endl;
			return { img_tensor.clone(), label_tensor.clone() };
		}
		torch::Tensor label_tensor = torch::zeros({ box_num ,5 }).to(torch::kFloat32);
		for (int i = 0; i < box_num; i++)
		{
            label_tensor[i][2] = boxes[i].w;// .GetW()* width_under1;
            label_tensor[i][3] = boxes[i].h;// GetH()* height_under1;
            label_tensor[i][0] = boxes[i].cx;// xmin* width_under1 + label_tensor[i][2] / 2;
            label_tensor[i][1] = boxes[i].cy;// ymin* height_under1 + label_tensor[i][3] / 2;
            label_tensor[i][4] = boxes[i].class_id;// name_idx.at(m_data.bboxes[i].name);
		}
		return { img_tensor.clone(), label_tensor.clone() };
	};

	std::vector<ExampleType> get_batch(c10::ArrayRef<size_t> indices) override {
		std::vector<ExampleType> batch;
		batch.reserve(indices.size());
		for (const auto i : indices) {
			batch.push_back(get(i));
		}
		return batch;
	}

	// Return the length of data
	torch::optional<size_t> size() const override {
        return _train_datas.size();
	};
};

//traverse all the .xml files and obtain the according images
//train --images
//      --labels
//val  --images
//     --labels
void load_det_data_from_folder(std::shared_ptr<Exporter> exporter,
                               const std::string &data_dir,
                               std::vector<TrainDataset> &train_datas,
                               std::vector<TrainDataset> &val_datas,
                               std::vector<int> ratio={9,3},
                               int batch_size=4,
                               bool use_same_data=true);
