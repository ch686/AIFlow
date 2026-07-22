#include "yolo_detect.h"
//#include "global_func.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
int yolo_detect::have_cuda=0;

yolo_detect::yolo_detect()
{
    m_DeviceType=torch::kCPU;
    m_ImgW=640;
    m_ImgH=640;
}
#ifdef CV_CUDA
float yolo_detect::letterbox_gpu(const cv::cuda::GpuMat &input_image, cv::cuda::GpuMat &output_image, const std::vector<int> &target_size)
{
    if (input_image.cols == target_size[1] && input_image.rows == target_size[0]) {
        if (input_image.data == output_image.data) {
            return 1.;
        } else {
            output_image = input_image.clone();
            return 1.;
        }
    }

    float resize_scale = generate_scale(input_image.size(), target_size);
    int new_shape_w = std::round(input_image.cols * resize_scale);
    int new_shape_h = std::round(input_image.rows * resize_scale);
    float padw = (target_size[1] - new_shape_w) / 2.;
    float padh = (target_size[0] - new_shape_h) / 2.;

    int top = std::round(padh - 0.1);
    int bottom = std::round(padh + 0.1);
    int left = std::round(padw - 0.1);
    int right = std::round(padw + 0.1);

    cv::cuda::resize(input_image, output_image,
               cv::Size(new_shape_w, new_shape_h),
               0, 0, cv::INTER_AREA);

    cv::cuda::copyMakeBorder(output_image, output_image, top, bottom, left, right,
                       cv::BORDER_CONSTANT, cv::Scalar(114.));
    return resize_scale;
}
#endif
float yolo_detect::generate_scale(const cv::Size& image_size, const std::vector<int>& target_size)
{
    int origin_w = image_size.width;
    int origin_h = image_size.height;

    int target_h = target_size[0];
    int target_w = target_size[1];

    float ratio_h = static_cast<float>(target_h) / static_cast<float>(origin_h);
    float ratio_w = static_cast<float>(target_w) / static_cast<float>(origin_w);
    float resize_scale = std::min(ratio_h, ratio_w);
    return resize_scale;
}


float yolo_detect::letterbox(const cv::Mat &input_image, cv::Mat &output_image, const std::vector<int> &target_size)
{
    if (input_image.cols == target_size[1] && input_image.rows == target_size[0]) {
        if (input_image.data == output_image.data) {
            return 1.;
        } else {
            output_image = input_image.clone();
            return 1.;
        }
    }

    float resize_scale = generate_scale(input_image.size(), target_size);
    int new_shape_w = std::round(input_image.cols * resize_scale);
    int new_shape_h = std::round(input_image.rows * resize_scale);
    float padw = (target_size[1] - new_shape_w) / 2.;
    float padh = (target_size[0] - new_shape_h) / 2.;

    int top = std::round(padh - 0.1);
    int bottom = std::round(padh + 0.1);
    int left = std::round(padw - 0.1);
    int right = std::round(padw + 0.1);

    cv::resize(input_image, output_image,
               cv::Size(new_shape_w, new_shape_h),
               0, 0, cv::INTER_AREA);

    cv::copyMakeBorder(output_image, output_image, top, bottom, left, right,
                       cv::BORDER_CONSTANT, cv::Scalar(114.));
    return resize_scale;
}

torch::Tensor yolo_detect::xyxy2xywh(const torch::Tensor& x)
{
    auto y = torch::empty_like(x);
    y.index_put_({"...", 0}, (x.index({"...", 0}) + x.index({"...", 2})).div(2));
    y.index_put_({"...", 1}, (x.index({"...", 1}) + x.index({"...", 3})).div(2));
    y.index_put_({"...", 2}, x.index({"...", 2}) - x.index({"...", 0}));
    y.index_put_({"...", 3}, x.index({"...", 3}) - x.index({"...", 1}));
    return y;
}


torch::Tensor yolo_detect::xywh2xyxy(const torch::Tensor& x)
{
    auto y = torch::empty_like(x);
    auto dw = x.index({"...", 2}).div(2);
    auto dh = x.index({"...", 3}).div(2);
    y.index_put_({"...", 0}, x.index({"...", 0}) - dw);
    y.index_put_({"...", 1}, x.index({"...", 1}) - dh);
    y.index_put_({"...", 2}, x.index({"...", 0}) + dw);
    y.index_put_({"...", 3}, x.index({"...", 1}) + dh);
    return y;
}


// Reference: https://github.com/pytorch/vision/blob/main/torchvision/csrc/ops/cpu/nms_kernel.cpp
torch::Tensor yolo_detect::nms(const torch::Tensor& bboxes, const torch::Tensor& scores, float iou_threshold)
{
    if (bboxes.numel() == 0)
        return torch::empty({0}, bboxes.options().dtype(torch::kLong));

    auto x1_t = bboxes.select(1, 0).contiguous();
    auto y1_t = bboxes.select(1, 1).contiguous();
    auto x2_t = bboxes.select(1, 2).contiguous();
    auto y2_t = bboxes.select(1, 3).contiguous();

    torch::Tensor areas_t = (x2_t - x1_t) * (y2_t - y1_t);

    auto order_t = std::get<1>(
        scores.sort(/*stable=*/true, /*dim=*/0, /* descending=*/true));

    auto ndets = bboxes.size(0);
    torch::Tensor suppressed_t = torch::zeros({ndets}, bboxes.options().dtype(torch::kByte));
    torch::Tensor keep_t = torch::zeros({ndets}, bboxes.options().dtype(torch::kLong));

    auto suppressed = suppressed_t.data_ptr<uint8_t>();
    auto keep = keep_t.data_ptr<int64_t>();
    auto order = order_t.data_ptr<int64_t>();
    auto x1 = x1_t.data_ptr<float>();
    auto y1 = y1_t.data_ptr<float>();
    auto x2 = x2_t.data_ptr<float>();
    auto y2 = y2_t.data_ptr<float>();
    auto areas = areas_t.data_ptr<float>();

    int64_t num_to_keep = 0;

    for (int64_t _i = 0; _i < ndets; _i++)
    {
        auto i = order[_i];
        if (suppressed[i] == 1)
            continue;
        keep[num_to_keep++] = i;
        auto ix1 = x1[i];
        auto iy1 = y1[i];
        auto ix2 = x2[i];
        auto iy2 = y2[i];
        auto iarea = areas[i];

        for (int64_t _j = _i + 1; _j < ndets; _j++)
        {
            auto j = order[_j];
            if (suppressed[j] == 1)
                continue;
            auto xx1 = std::max(ix1, x1[j]);
            auto yy1 = std::max(iy1, y1[j]);
            auto xx2 = std::min(ix2, x2[j]);
            auto yy2 = std::min(iy2, y2[j]);

            auto w = std::max(static_cast<float>(0), xx2 - xx1);
            auto h = std::max(static_cast<float>(0), yy2 - yy1);
            auto inter = w * h;
            auto ovr = inter / (iarea + areas[j] - inter);
            if (ovr > iou_threshold)
                suppressed[j] = 1;
        }
    }
    return keep_t.narrow(0, 0, num_to_keep);
}


std::vector<torch::Tensor> yolo_detect::non_max_supperession(torch::Tensor& prediction,float conf_thres, float iou_thres, int max_det)
{
    //output:[1, 12, 8400]
    //output:[1, 56, 8400]
    int bs = prediction.size(0);
    int nc = m_ClassMap.size()?m_ClassMap.size():prediction.size(1) - 4;
    int nm = prediction.size(1) - nc - 4;
    auto mi = 4 + nc;
    auto xc = prediction.index({torch::indexing::Slice(), torch::indexing::Slice(4, mi)}).amax(1) > conf_thres;

    prediction = prediction.transpose(-1, -2);
    prediction.index_put_({"...", torch::indexing::Slice({torch::indexing::None, 4})}, xywh2xyxy(prediction.index({"...", torch::indexing::Slice(torch::indexing::None, 4)})));

    std::vector<torch::Tensor> output;
    for (int i = 0; i < bs; i++)
    {
        output.push_back(torch::zeros({0, 6 + nm}, prediction.device()));
    }

    for (int xi = 0; xi < prediction.size(0); xi++)
    {
        auto x = prediction[xi];
        x = x.index({xc[xi]});
        auto x_split = x.split({4, nc, nm}, 1);
        auto box = x_split[0];
        auto cls = x_split[1];
        auto mask = x_split[2];

        //auto [conf, j] = cls.max(1, true);
        //std::tuple
        std::tuple<torch::Tensor,torch::Tensor> result = cls.max(1, true);
        auto conf = std::get<0>(result);
        auto j =  std::get<1>(result);

        x = torch::cat({box, conf, j.toType(torch::kFloat), mask}, 1);
        x = x.index({conf.view(-1) > conf_thres});
        int n = x.size(0);
        if (!n)
        {
            continue;
        }

        // NMS
        auto c = x.index({torch::indexing::Slice(), torch::indexing::Slice{5, 6}}) * 7680;
        auto boxes = x.index({torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None, 4)}) + c;
        auto scores = x.index({torch::indexing::Slice(), 4});
        auto i = nms(boxes.cpu(), scores.cpu(), iou_thres);
        i = i.index({torch::indexing::Slice(torch::indexing::None, max_det)});
        output[xi] = x.index({i});

      //  std::cout<<output[xi].type()<<std::endl;
      //  std::cout<<output[xi].sizes()<<std::endl;

    }

    //torch::Tensor outputs=torch::stack(output);

    return output;
}


torch::Tensor yolo_detect::clip_boxes(torch::Tensor& boxes, const std::vector<int>& shape)
{
    boxes.index_put_({"...", 0}, boxes.index({"...", 0}).clamp(0, shape[1]));
    boxes.index_put_({"...", 1}, boxes.index({"...", 1}).clamp(0, shape[0]));
    boxes.index_put_({"...", 2}, boxes.index({"...", 2}).clamp(0, shape[1]));
    boxes.index_put_({"...", 3}, boxes.index({"...", 3}).clamp(0, shape[0]));
    return boxes;
}


torch::Tensor yolo_detect::scale_boxes(const std::vector<int>& img1_shape, torch::Tensor& boxes, const std::vector<int>& img0_shape)
{
    //std::cout<<"boxes:"<<boxes.sizes()<<std::endl;
    auto gain = (std::min)((float)img1_shape[0] / img0_shape[0], (float)img1_shape[1] / img0_shape[1]);
    auto pad0 = std::round((float)(img1_shape[1] - img0_shape[1] * gain) / 2. - 0.1);
    auto pad1 = std::round((float)(img1_shape[0] - img0_shape[0] * gain) / 2. - 0.1);
    if(boxes.size(1)==4)
    {
        boxes.index_put_({"...", 0}, boxes.index({"...", 0}) - pad0);
        boxes.index_put_({"...", 2}, boxes.index({"...", 2}) - pad0);
        boxes.index_put_({"...", 1}, boxes.index({"...", 1}) - pad1);
        boxes.index_put_({"...", 3}, boxes.index({"...", 3}) - pad1);
        boxes.index_put_({"...", torch::indexing::Slice(torch::indexing::None, 4)}, boxes.index({"...", torch::indexing::Slice(torch::indexing::None, 4)}).div(gain));
    }
    else if(boxes.size(1)==2)
    {
        boxes.index_put_({"...", 0}, boxes.index({"...", 0}) - pad0);
        boxes.index_put_({"...", 1}, boxes.index({"...", 1}) - pad1);
        boxes.index_put_({"...", torch::indexing::Slice(torch::indexing::None, 2)}, boxes.index({"...", torch::indexing::Slice(torch::indexing::None, 2)}).div(gain));
    }
    return boxes;
}

int yolo_detect::is_cuda_available()
{
    have_cuda= torch::cuda::is_available();
    printf("torch::cuda::is_available=%d\n",have_cuda);
    int is_cdua_cudnn= torch::cuda::cudnn_is_available();// << std::endl;
    printf("torch::cuda::cudnn_is_available=%d\n",is_cdua_cudnn);
    int cdua_device=torch::cuda::device_count();// << std::endl;
    printf("torch::cuda::device_count=%d\n",cdua_device);

    return have_cuda;
}

int yolo_detect::load_model(const std::string &model_path)
{

    try
    {
        // yolo_model = torch::jit::load(model_path);
         // 附加文件路径
          std::string extra_path = "config.txt";
          // 创建ExtraFilesMap对象
          torch::jit::ExtraFilesMap extra_files_map;
          extra_files_map.emplace(extra_path, "");
          yolo_model = torch::jit::load(model_path, c10::nullopt,extra_files_map);  //加载模型
          std::string extra_text=extra_files_map["config.txt"];
        #ifdef _DEBUG
          std::cout<<extra_text<<std::endl;
        #endif
          if(extra_text.size()==0)
          {
              printf("[E] error loading the extra_text\n");

              return 0;
          }

          // 1. 字符串转文档
          QJsonParseError err;
          QJsonDocument doc = QJsonDocument::fromJson(extra_text.c_str(), &err);
          if (err.error != QJsonParseError::NoError)
          {
              qDebug() << "JSON解析错误：" << err.errorString();
              return 0;
          }

          QJsonObject obj = doc.object();

          // 3. 读取字段
          QJsonArray imgsz = obj["imgsz"].toArray();
          QJsonArray kpt_shape = obj["kpt_shape"].toArray();
          int kpt_size=kpt_shape.size();
          if(kpt_size>0)
          {
              m_kpt_shape.resize(kpt_size);
              for(int i=0;i<kpt_size;i++)
                  m_kpt_shape[i]=kpt_shape[i].toInt();
          }
          if(imgsz.size()>=2)
          {
              m_ImgW=imgsz[1].toInt();
              m_ImgH=imgsz[0].toInt();
          }
          else
          {
              m_ImgW=640;
              m_ImgH=640;
          }


          printf("[m] model_size:%dx%d\n",m_ImgW,m_ImgH);
          QJsonObject name_obj = obj["names"].toObject();

          for(int i=0;i<name_obj.keys().size();i++)
          {
              int clsid=name_obj.keys()[i].toInt();
              m_ClassMap[clsid]=name_obj[name_obj.keys()[i]].toString().toUtf8().constData();
          }

          printf("[m] class_num:%d\n",m_ClassMap.size());

    }
    catch (const c10::Error& e)
    {
        printf("[E] error loading the model:%s\n",e.msg().c_str());
        return 0;
    }

    // 检查模型参数的设备类型
    bool isCuda = false;
    for (const auto& parameter : yolo_model.parameters())
    {
        if (parameter.device().is_cuda())
        {
            isCuda = true;
            break;
        }
    }
    if(have_cuda&&isCuda)
    {
        m_DeviceType=torch::kCUDA;
    }
    else
    {
        if(isCuda)
        {
            printf("[E] error loading the gpu model:%s\n",model_path.c_str());
            return 0;
        }
        m_DeviceType=torch::kCPU;
    }
    torch::Device device(m_DeviceType);

    yolo_model.eval();
    yolo_model.to(device, torch::kFloat32);




    return 1;
}
#ifdef CV_CUDA
int yolo_detect::detect_gpu(const std::vector<cv::cuda::GpuMat> &images)
{
    m_DetResults.clear();
    if (images.size() == 0)
        return 0;

    // Load image and preprocess
     // cv::Mat image = cv::imread("/path/to/bus.jpg");
    torch::Device device(m_DeviceType);

    std::vector<torch::Tensor> input_imgs;
    for (auto& img : images)
    {
        cv::cuda::GpuMat input_image;
        letterbox_gpu(img, input_image, {m_ImgH,m_ImgW });
        cv::cuda::cvtColor(input_image, input_image, cv::COLOR_BGR2RGB);

        cv::cuda::GpuMat continuousGpuMat;
        cv::cuda::createContinuous(input_image.size(), input_image.type(), continuousGpuMat);
        input_image.copyTo(continuousGpuMat);

        auto options = torch::TensorOptions().dtype(torch::kByte).device(torch::kCUDA);
        torch::Tensor image_tensor = torch::from_blob(continuousGpuMat.data,
            { continuousGpuMat.rows, continuousGpuMat.cols, continuousGpuMat.channels() }, options);

        image_tensor = image_tensor.toType(torch::kFloat32).div(255);
        image_tensor = image_tensor.permute({ 2, 0, 1 });
        image_tensor = image_tensor.unsqueeze(0);
        // std::cout<<image_tensor.type()<<std::endl;
        // std::cout<<image_tensor.sizes()<<std::endl;
        input_imgs.push_back(image_tensor);
    }
    // Inference
    torch::Tensor inputs = torch::cat(input_imgs, 0);
    //std::cout<<"yolo_model inputs.sizes:"<<inputs.sizes()<<std::endl;

    torch::Tensor output = yolo_model.forward({ inputs }).toTensor();
    //std::cout<<"output:"<<output<<std::endl;
   //output:[1, 12, 8400]
   //output:[1, 56, 8400]
   //std::cout<<"output:"<<output.sizes()<<std::endl;
   // NMS
    std::vector<torch::Tensor>  keeps = non_max_supperession(output);//[0];
    for (int i = 0; i < keeps.size(); i++)
    {
        torch::Tensor  keep = keeps[i];
        //std::cout<<"keep:"<<keep<<std::endl;
        torch::Tensor boxes = keep.index({ torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None, 4) });
        // std::cout<<"boxes:"<<boxes<<std::endl;
        const cv::cuda::GpuMat &image = images.at(i);
        //std::cout<<boxes<<std::endl;
        //std::cout<<boxes.sizes()<<std::endl;

        torch::Tensor boxes_scale = scale_boxes({ m_ImgH, m_ImgW }, boxes, { image.rows, image.cols });
        // std::cout<<"boxes_scale:"<<boxes_scale<<std::endl;

        keep.index_put_({ torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None, 4) }, boxes_scale);
        // std::cout<<"keep:"<<keep<<std::endl;
        if (m_kpt_shape.size() > 1)
        {
            for (int i = 0; i < m_kpt_shape[0]; i++)
            {
                torch::Tensor point = keep.index({ torch::indexing::Slice(), torch::indexing::Slice(6 + i * 3, 8 + i * 3) });
                torch::Tensor point_scale = scale_boxes({ m_ImgH, m_ImgW  }, point, { image.rows, image.cols });
                keep.index_put_({ torch::indexing::Slice(), torch::indexing::Slice(6 + i * 3, 8 + i * 3) }, point_scale);
            }
        }

        // Show the results
        std::vector<YOLO_RESULT> dets;
        for (int i = 0; i < keep.size(0); i++)
        {
            float x1 = keep[i][0].item().toFloat();
            float y1 = keep[i][1].item().toFloat();
            float x2 = keep[i][2].item().toFloat();
            float y2 = keep[i][3].item().toFloat();
            float conf = keep[i][4].item().toFloat();
            int cls = keep[i][5].item().toInt();

            YOLO_RESULT det;
            det.box.width = (x2 - x1) / image.cols;
            det.box.height = (y2 - y1) / image.rows;
            det.box.x = x1 / image.cols + det.box.width / 2;
            det.box.y = y1 / image.rows + det.box.height / 2;
            det.conf = conf;
            det.classid = cls;
            if (m_ClassMap.find(cls) != m_ClassMap.end())
                det.classname = m_ClassMap[cls];
            //std::cout<<"keep:"<<keep<<std::endl;
            if (m_kpt_shape.size() > 1)
            {
                for (int j = 0; j < m_kpt_shape[0]; j++)
                {
                    POSE_PT pt;
                    pt.x = keep[i][6 + j * 3].item().toFloat() / image.cols;
                    pt.y = keep[i][7 + j * 3].item().toFloat() / image.rows;
                    pt.conf = keep[i][8 + j * 3].item().toFloat();
                    det.points.push_back(pt);
                }
            }


            dets.push_back(det);
            //std::cout << "Rect: [" << x1 << "," << y1 << "," << x2 << "," << y2 << "]  Conf: " << conf << "  Class: " << cls<< std::endl;
}

        m_DetResults.push_back(dets);
    }
    return m_DetResults.size();
}
#endif
int yolo_detect::detect(const std::vector<cv::Mat> &images)
{
    m_DetResults.clear();
    if(images.size()==0)
        return 0;

    // Load image and preprocess
     // cv::Mat image = cv::imread("/path/to/bus.jpg");
    torch::Device device(m_DeviceType);

    std::vector<torch::Tensor> input_imgs;
#ifdef CV_CUDA
    if (m_HostmemsInput.size() != images.size())
    {
        m_HostmemsInput.clear();
        m_HostmemsInput.resize(images.size());
    }
#endif
    for(int i=0;i<images.size();i++)
    {
        const cv::Mat& img = images.at(i);
#ifdef CV_CUDA
        if (m_HostmemsInput[i] == nullptr)
        {
            m_HostmemsInput[i] = std::make_shared<cv::cuda::HostMem>(img.size(), img.type(), cv::cuda::HostMem::SHARED);
        }
        else
        {
            cv::Mat hmCpu = m_HostmemsInput[i]->createMatHeader();
            if (hmCpu.size() != img.size())
            {
                m_HostmemsInput[i].reset();
                m_HostmemsInput[i] = std::make_shared<cv::cuda::HostMem>(img.size(), img.type(), cv::cuda::HostMem::SHARED);
            }
        }
        img.copyTo(*m_HostmemsInput[i]);
        cv::cuda::GpuMat hmGpu = m_HostmemsInput[i]->createGpuMatHeader();
        cv::cuda::GpuMat input_image;
        letterbox_gpu(hmGpu, input_image, { m_ImgH,m_ImgW });
        cv::cuda::cvtColor(input_image, input_image, cv::COLOR_BGR2RGB);
        cv::cuda::GpuMat continuousGpuMat;
        cv::cuda::createContinuous(input_image.size(), input_image.type(), continuousGpuMat);
        input_image.copyTo(continuousGpuMat);

        auto options = torch::TensorOptions().dtype(torch::kByte).device(torch::kCUDA);
        torch::Tensor image_tensor = torch::from_blob(continuousGpuMat.data, 
            { continuousGpuMat.rows, continuousGpuMat.cols, continuousGpuMat.channels() }, options);

#else
        cv::Mat input_image;
        letterbox(img, input_image, { m_ImgH, m_ImgW  });
        cv::cvtColor(input_image, input_image, cv::COLOR_BGR2RGB);
        torch::Tensor image_tensor = torch::from_blob(input_image.data, {input_image.rows, input_image.cols, 3}, torch::kByte).to(device);
#endif
        image_tensor = image_tensor.toType(torch::kFloat32).div(255);
        image_tensor = image_tensor.permute({2, 0, 1});
        image_tensor = image_tensor.unsqueeze(0);
       // std::cout<<image_tensor.type()<<std::endl;
       // std::cout<<image_tensor.sizes()<<std::endl;
        input_imgs.push_back(image_tensor);
    }
    // Inference
    torch::Tensor inputs=torch::cat(input_imgs,0);
    //std::cout<<"yolo_model inputs.sizes:"<<inputs.sizes()<<std::endl;

    torch::Tensor output = yolo_model.forward({inputs}).toTensor();
     //std::cout<<"output:"<<output<<std::endl;
    //output:[1, 12, 8400]
    //output:[1, 56, 8400]
    //std::cout<<"output:"<<output.sizes()<<std::endl;
    // NMS
    std::vector<torch::Tensor>  keeps = non_max_supperession(output);//[0];
    for(int i=0;i<keeps.size();i++)
    {
        torch::Tensor  keep=keeps[i];
        //std::cout<<"keep:"<<keep<<std::endl;
        torch::Tensor boxes = keep.index({torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None, 4)});
       // std::cout<<"boxes:"<<boxes<<std::endl;
        cv::Mat image=images.at(i);
        //std::cout<<boxes<<std::endl;
        //std::cout<<boxes.sizes()<<std::endl;

        torch::Tensor boxes_scale=scale_boxes({m_ImgW, m_ImgH}, boxes, {image.rows, image.cols});
       // std::cout<<"boxes_scale:"<<boxes_scale<<std::endl;

        keep.index_put_({torch::indexing::Slice(), torch::indexing::Slice(torch::indexing::None, 4)},boxes_scale);
       // std::cout<<"keep:"<<keep<<std::endl;
        if(m_kpt_shape.size()>1)
        {
            for(int i=0;i<m_kpt_shape[0];i++)
            {
                torch::Tensor point = keep.index({torch::indexing::Slice(), torch::indexing::Slice(6+i*3, 8+i*3)});
                torch::Tensor point_scale=scale_boxes({m_ImgW, m_ImgH}, point, {image.rows, image.cols});
                keep.index_put_({torch::indexing::Slice(), torch::indexing::Slice(6+i*3, 8+i*3)},point_scale);
            }
        }

        // Show the results
        std::vector<YOLO_RESULT> dets;
        for (int i = 0; i < keep.size(0); i++)
        {
            float x1 = keep[i][0].item().toFloat();
            float y1 = keep[i][1].item().toFloat();
            float x2 = keep[i][2].item().toFloat();
            float y2 = keep[i][3].item().toFloat();
            float conf = keep[i][4].item().toFloat();
            int cls = keep[i][5].item().toInt();

            YOLO_RESULT det;
            det.box.width=(x2-x1)/image.cols;
            det.box.height=(y2-y1)/image.rows;
            det.box.x=x1/image.cols+det.box.width/2;
            det.box.y=y1/image.rows+det.box.height/2;
            det.conf= conf;
            det.classid=cls;
            if(m_ClassMap.find(cls)!=m_ClassMap.end())
                det.classname=m_ClassMap[cls];
            //std::cout<<"keep:"<<keep<<std::endl;
            if(m_kpt_shape.size()>1)
            {
                for(int j=0;j<m_kpt_shape[0];j++)
                {
                    POSE_PT pt;
                    pt.x= keep[i][6+j*3].item().toFloat()/image.cols;
                    pt.y= keep[i][7+j*3].item().toFloat()/image.rows;
                    pt.conf= keep[i][8+j*3].item().toFloat();
                    det.points.push_back(pt);
                }
            }


            dets.push_back(det);
            //std::cout << "Rect: [" << x1 << "," << y1 << "," << x2 << "," << y2 << "]  Conf: " << conf << "  Class: " << cls<< std::endl;
        }

        m_DetResults.push_back(dets);
    }
    return m_DetResults.size();

}

std::vector<YOLO_RESULT> yolo_detect::get_result(int index)
{
    std::vector<YOLO_RESULT> dets;
    if(index>=0&&index<m_DetResults.size())
    {
        dets=m_DetResults.at(index);
    }
    return dets;
}

std::string yolo_detect::get_class_name(int id)
{
    if(m_ClassMap.find(id)==m_ClassMap.end())
        return "";
    return m_ClassMap[id];
}

std::vector<std::string> yolo_detect::get_class_names()
{
    std::vector<std::string> names;
    for(int i=0;i<m_ClassMap.size();i++)
    {
        names.push_back(get_class_name(i));
    }
    return names;
}


void yolo_detect::test(int bs)
{
    torch::Device device(m_DeviceType);
    torch::Tensor input_tensor = torch::rand({ bs,3,m_ImgH ,m_ImgW }, torch::kFloat32).to(device);
    torch::jit::IValue output = yolo_model.forward({ input_tensor });
}
