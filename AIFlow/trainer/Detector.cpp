#include "Detector.h"
#include"net/yolo_training.h"
//#include <sys/stat.h>
#include"net_utils/Augmentations.h"
cv::Ptr<cv::freetype::FreeType2> Detector::_ftFont2=nullptr;

Detector::Detector(QObject *parent)
{
    if(!_ftFont2)
    {
        _ftFont2 =cv::freetype::createFreeType2();
        _ftFont2->loadFontData( "C:/Windows/Fonts/simhei.ttf", 0 );
    }
}

void Detector::InitializeTrain(int gpu_id, int width, int height,
                          std::shared_ptr<Exporter> exporter,
                          const std::string &data_dir)
{
    _exporter=exporter;
    _data_dir=data_dir;
    if (gpu_id >= 0)
    {
        if (gpu_id >= torch::getNumGPUs())
        {
			std::cout << "No GPU id " << gpu_id << " abailable" << std::endl;
            _device = torch::Device(torch::kCPU);
		}
		else
        {
            _device = torch::Device(torch::kCUDA, gpu_id);
        }
	}
    else
    {
        _device = torch::Device(torch::kCPU);
	}
    int num_classes=80;
    if(_exporter!=nullptr)
    {
        _name_list.clear();
        QStringList qList = exporter->get_class_names();
        num_classes=qList.size();
        _name_list.resize(num_classes);
        for(int i=0;i<num_classes;i++)
        {
            _name_list[i]=qList[i].toUtf8().constData();
        }
    }

    _width = width;
    _height = height;
    if (_width % 32 || _height % 32)
    {
		std::cout << "Width or height is not divisible by 32" << std::endl;
		return ;
	}

    _detector = YoloBody_tiny(3, num_classes);
    _detector->to(_device);
	return;
}

void Detector::InitializePredict(int gpu_id, int width, int height,
                                 const QStringList &class_list)
{
    if (gpu_id >= 0)
    {
        if (gpu_id >= torch::getNumGPUs())
        {
            std::cout << "No GPU id " << gpu_id << " abailable" << std::endl;
            _device = torch::Device(torch::kCPU);
        }
        else
        {
            _device = torch::Device(torch::kCUDA, gpu_id);
        }
    }
    else
    {
        _device = torch::Device(torch::kCPU);
    }

    int num_classes=class_list.size();
    _name_list.resize(num_classes);
    for(int i=0;i<num_classes;i++)
    {
        _name_list[i]=class_list[i].toUtf8().constData();
    }

    _width = width;
    _height = height;
    if (_width % 32 || _height % 32)
    {
        std::cout << "Width or height is not divisible by 32" << std::endl;
        return ;
    }

    _detector = YoloBody_tiny(3, num_classes);
    _detector->to(_device);
    return;
}


void Detector::loadPretrained(std::string pretrained_pth) {
	auto net_pretrained = YoloBody_tiny(3, 80);
	torch::load(net_pretrained, pretrained_pth);
    if (_name_list.size() == 80)
	{
        _detector = net_pretrained;
	}

	torch::OrderedDict<std::string, at::Tensor> pretrained_dict = net_pretrained->named_parameters();
    torch::OrderedDict<std::string, at::Tensor> model_dict = _detector->named_parameters();


	for (auto n = pretrained_dict.begin(); n != pretrained_dict.end(); n++)
	{
		if (strstr((*n).key().c_str(), "yolo_head")) {
			continue;
		}
		model_dict[(*n).key()] = (*n).value();
	}

	torch::autograd::GradMode::set_enabled(false);  // make parameters copying possible
	auto new_params = model_dict; // implement this
    auto params = _detector->named_parameters(true /*recurse*/);
    auto buffers = _detector->named_buffers(true /*recurse*/);
    for (auto& val : new_params)
    {
		auto name = val.key();
		auto* t = params.find(name);
        if (t != nullptr)
        {
			t->copy_(val.value());
		}
        else
        {
			t = buffers.find(name);
            if (t != nullptr)
            {
				t->copy_(val.value());
			}
		}
	}
	torch::autograd::GradMode::set_enabled(true);
}

inline bool does_exist(const std::string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

void Detector::Train()
{
    _is_finish=false;
    _stop_flag=false;
    float best_loss = 1e10;
    int num_epochs=_num_epochs;
    int batch_size=_batch_size;
    float learning_rate=_learning_rate;
    std::string gbk_data_dir=util::utf8_to_gbk(_data_dir.c_str());
    std::string pretrained_path=gbk_data_dir+"/.data/best.pt";
	if (!does_exist(pretrained_path))
	{
		std::cout << "Pretrained path is invalid: " << pretrained_path <<"\t random initialzed the model"<< std::endl;
	}
	else 
	{
		loadPretrained(pretrained_path);
        best_loss=_best_loss;
    }

	//the datasets must be the required structure
    //std::string train_label_path = train_val_path + "/train/labels";
    //std::string val_label_path = train_val_path + "/val/labels";

    std::vector<TrainDataset> list_train = {};
    std::vector<TrainDataset> list_val = {};

    load_det_data_from_folder(_exporter,_data_dir,list_train,list_val,{2,1},batch_size);

    if (list_train.size() < batch_size || list_val.size() < batch_size) {
		std::cout << "Image numbers less than batch size or empty image folder" << std::endl;
        emit sigTrainFinished();

		return;
	}

    auto custom_dataset_train = DetDataset(list_train,_name_list, true,_width, _height);
    auto custom_dataset_val = DetDataset(list_val,_name_list,  false, _width, _height);
	auto mdataset_train = custom_dataset_train.map(torch::data::transforms::Stack< >());
	auto mdataset_val = custom_dataset_val.map(torch::data::transforms::Stack< >());
	int total_train = mdataset_train.size().value();
	int total_val = mdataset_val.size().value();
	auto data_loader_train = torch::data::make_data_loader<torch::data::samplers::RandomSampler>(std::move(custom_dataset_train), batch_size);
	auto data_loader_val = torch::data::make_data_loader<torch::data::samplers::RandomSampler>(std::move(custom_dataset_val), batch_size);

	float anchor[12] = { 10,14,  23,27,  37,58,  81,82,  135,169,  344,319 };
    auto anchors_ = torch::from_blob(anchor, { 6,2 }, torch::TensorOptions(torch::kFloat32)).to(_device);
    int image_size[2] = { _width, _height };

	bool normalize = false;
    auto critia1 = YOLOLossImpl(anchors_, _name_list.size(), image_size, 0.01, _device, normalize);
    auto critia2 = YOLOLossImpl(anchors_, _name_list.size(), image_size, 0.01, _device, normalize);
	
    auto pretrained_dict = _detector->named_parameters();
    auto FloatType = torch::ones(1).to(torch::kFloat).to(_device).options();
    for (int epoc_count = 0; epoc_count <= num_epochs; epoc_count++)
    {
		float loss_sum = 0;
		int batch_count = 0;
        float loss_train = 0;
        float loss_val = 0;

        if (epoc_count == int(num_epochs / 2))
        {
            learning_rate /= 10;
        }
        torch::optim::Adam optimizer(_detector->parameters(), learning_rate); // Learning Rate
        if (epoc_count < int(num_epochs / 10))
        {
			for (auto mm : pretrained_dict)
			{
				if (strstr(mm.key().c_str(), "yolo_head"))
				{
					mm.value().set_requires_grad(true);
				}
				else
				{
					mm.value().set_requires_grad(false);
				}
			}
		}
        else
        {
            for (auto mm : pretrained_dict)
            {
				mm.value().set_requires_grad(true);
			}
		}
        _detector->train();
		//int total_batches = static_cast<int>(std::ceil(static_cast<double>(total_size) / batch_size));
		int batch_size_count = 0;
        for (auto& batch : *data_loader_train)
        {
			std::vector<torch::Tensor> images_vec = {};
			std::vector<torch::Tensor> targets_vec = {};
			int cur_batch_size = batch.size();
			//if (batch.size() < batch_size) continue;
			for (int i = 0; i < cur_batch_size; i++)
			{
				images_vec.push_back(batch[i].data.to(FloatType));
				targets_vec.push_back(batch[i].target.to(FloatType));
			}
			auto data = torch::stack(images_vec).div(255.0);

			optimizer.zero_grad();
            auto outputs = _detector->forward(data);
			std::vector<torch::Tensor> loss_numpos1 = critia1.forward(outputs[0], targets_vec);
			std::vector<torch::Tensor> loss_numpos2 = critia1.forward(outputs[1], targets_vec);

			auto loss = loss_numpos1[0] + loss_numpos2[0];
			auto num_pos = loss_numpos1[1] + loss_numpos2[1];
            if (num_pos.item<int64_t>() >0)
            {
                loss = loss / num_pos;
                //loss = loss / num_pos;
                loss.backward();
                loss_sum += loss.item().toFloat();
                batch_count++;
            }

			optimizer.step();

			batch_size_count += cur_batch_size;

            std::cout << "Epoch: " << epoc_count << "("<< batch_size_count <<"/"<< total_train <<")," << "\r";
            emit sigTrainProgress(true,epoc_count,num_epochs,batch_size_count,total_train);
            if (_stop_flag)
            {
                break;
            }
        }
        if (_stop_flag)
        {
            emit sigTrainFinished();
            return;
        }
        loss_train = loss_sum / batch_count;
        std::cout<<std::endl << " Train Loss: " << loss_train<<std::endl;
        emit sigTrainLoss(true,epoc_count,num_epochs,loss_train);

		std::cout << std::endl;
        _detector->eval();
		loss_sum = 0; batch_count = 0;
		batch_size_count = 0;
        for (auto& batch : *data_loader_val)
        {
			std::vector<torch::Tensor> images_vec = {};
			std::vector<torch::Tensor> targets_vec = {};
			int cur_batch_size = batch.size();
			//if (batch.size() < batch_size) continue;
			for (int i = 0; i < cur_batch_size; i++)
			{
				images_vec.push_back(batch[i].data.to(FloatType));
				targets_vec.push_back(batch[i].target.to(FloatType));
			}
			auto data = torch::stack(images_vec).div(255.0);

            auto outputs = _detector->forward(data);
            std::vector<torch::Tensor> loss_numpos1 = critia1.forward(outputs[0], targets_vec);
            std::vector<torch::Tensor> loss_numpos2 = critia1.forward(outputs[1], targets_vec);
			auto loss = loss_numpos1[0] + loss_numpos2[0];
			auto num_pos = loss_numpos1[1] + loss_numpos2[1];

            if (num_pos.item<int64_t>() >0)
            {
                loss = loss / num_pos;
                loss_sum += loss.item<float>();
                batch_count++;
            }
			batch_size_count += cur_batch_size;

            std::cout << "Epoch: " << epoc_count << "(" << batch_size_count << "/" << total_val << ")," << "\r";
            emit sigTrainProgress(false,epoc_count,num_epochs,batch_size_count,total_val);
            // ========= 关键：检测停止标记 =========
            if (_stop_flag)
            {
                break;
            }
        }
        if (_stop_flag)
        {
            emit sigTrainFinished();
            return;
        }
        loss_val = loss_sum / batch_count;
        std::cout<<std::endl << " Valid Loss: " << loss_val<<std::endl;
        emit sigTrainLoss(false,epoc_count,num_epochs,loss_val);

		printf("\n");
		if (best_loss >= loss_val) 
		{
			best_loss = loss_val;
            torch::save(_detector, gbk_data_dir+"/.data/best.pt");
            // 写入best_loss到txt
            std::string best_loss_file = gbk_data_dir + "/.data/best_loss.txt";
            std::ofstream best_out(best_loss_file);
            if(best_out.is_open())
            {
                best_out << best_loss;
                best_out.close();
            }

		}
        torch::save(_detector,gbk_data_dir+"/.data/last.pt");
        std::string last_loss_file = gbk_data_dir + "/.data/last_loss.txt";
        std::ofstream last_out(last_loss_file);
        if(last_out.is_open())
        {
            last_out << loss_val;
            last_out.close();
        }

	}
    _is_finish=true;
    emit sigTrainFinished();
}

void Detector::LoadWeight(std::string weight_path) {
	try
	{
        torch::load(_detector, weight_path);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what();
	}
    _detector->to(_device);
    _detector->eval();
	return;
}

void Detector::show_bbox(cv::Mat image, torch::Tensor bboxes,
               std::vector<std::string> name_list,bool bShow)
{

	int font_face = cv::FONT_HERSHEY_COMPLEX;
	double font_scale = 0.4;
	int thickness = 1;
	std::cout << bboxes << std::endl;
    if (bboxes.equal(torch::zeros_like(bboxes)))
        return;
    float* bbox = new float[bboxes.size(0)]();
    memcpy(bbox, bboxes.cpu().data_ptr(), bboxes.size(0) * sizeof(float));
	for (int i = 0; i < bboxes.size(0); i = i + 7)
	{
		cv::rectangle(image, cv::Rect(bbox[i + 0], bbox[i + 1], bbox[i + 2] - bbox[i + 0], bbox[i + 3] - bbox[i + 1]), cv::Scalar(0, 0, 255));

		cv::Point origin;
		origin.x = bbox[i + 0];
		origin.y = bbox[i + 1] + 8;
        _ftFont2->putText(image, name_list[bbox[i + 6]],
                origin, 24, cv::Scalar(0, 0, 255), -1, 8, true);

        //cv::putText(image, name_list[bbox[i + 6]], origin, font_face, font_scale, cv::Scalar(0, 0, 255), thickness, 1, 0);
	}
	delete bbox;
    if(bShow)
    {
        cv::imwrite("prediction.jpg", image);
        cv::imshow("test", image);
        cv::waitKey(0);
        cv::destroyAllWindows();
    }
}

void Detector::Predict(cv::Mat &_image,
                       bool show,float conf_thresh, float nms_thresh)
{
    cv::Mat image=_image.clone();
	int origin_width = image.cols;
	int origin_height = image.rows;
    cv::resize(image, image, { _width,_height });
	auto img_tensor = torch::from_blob(image.data, { image.rows, image.cols, 3 }, torch::kByte);
	img_tensor = img_tensor.permute({ 2, 0, 1 }).unsqueeze(0).to(torch::kFloat) / 255.0;

	float anchor[12] = { 10,14,  23,27,  37,58,  81,82,  135,169,  344,319 };
	auto anchors_ = torch::from_blob(anchor, { 6,2 }, torch::TensorOptions(torch::kFloat32));
    int image_size[2] = { _width,_height };
    img_tensor = img_tensor.to(_device);

    auto outputs = _detector->forward(img_tensor);
	std::vector<torch::Tensor> output_list = {};
	auto tensor_input = outputs[1];
    auto output_decoded = DecodeBox(tensor_input, anchors_.narrow(0, 0, 3), _name_list.size(), image_size);
	output_list.push_back(output_decoded);

	tensor_input = outputs[0];
    output_decoded = DecodeBox(tensor_input, anchors_.narrow(0, 3, 3), _name_list.size(), image_size);
	output_list.push_back(output_decoded);

	//std::cout << tensor_input << anchors_.narrow(0, 3, 3);

	auto output = torch::cat(output_list, 1);
    auto detection = non_maximum_suppression(output, _name_list.size(), conf_thresh, nms_thresh);

    float w_scale = float(origin_width) / _width;
    float h_scale = float(origin_height) / _height;
	for (int i = 0; i < detection.size(); i++) {
		for (int j = 0; j < detection[i].size(0) / 7; j++)
		{
			detection[i].select(0, 7 * j + 0) *= w_scale;
			detection[i].select(0, 7 * j + 1) *= h_scale;
			detection[i].select(0, 7 * j + 2) *= w_scale;
			detection[i].select(0, 7 * j + 3) *= h_scale;
		}
	}

    //cv::resize(image, image, { origin_width,origin_height });
    show_bbox(_image, detection[0], _name_list,show);
//    else
//    {
//        torch::Tensor bboxes=detection[0];
//        if (bboxes.equal(torch::zeros_like(bboxes)))
//            return;
//        float* bbox = new float[bboxes.size(0)];
//        memcpy(bbox, bboxes.cpu().data_ptr(), bboxes.size(0) * sizeof(float));
//        for (int i = 0; i < bboxes.size(0); i = i + 7)
//        {
//            BBox box;
//            box.class_id=bbox[i + 6];
//            box.cx=bbox[i + 0];
//            box.cy=bbox[i + 1];
//            box.w= bbox[i + 2] - bbox[i + 0];
//            box.h= bbox[i + 3] - bbox[i + 1];
//            boxes.push_back(box);
//        }
//        delete []bbox;
//    }
	return;
}

void Detector::StopTrain()
{
    _stop_flag = true;
}
