# AIFlow — 可视化 AI 流程编辑器

基于节点图的计算机视觉 / 深度学习可视化编排工具。通过拖拽节点、连线的方式构建「图像采集 → 模型推理 / 训练 → 结果展示」的完整 AI 工作流，无需编写代码。

内置 YOLO 目标检测的训练与推理引擎、图像标注工具，以及手势识别 / 人脸识别等预训练模型。

---

## 功能特性

### 节点式可视化编程
- 左侧节点树按分类组织，双击或拖拽即可创建节点
- 画布上拖动端口连线，构建数据流向（图像 → 模型 → 输出）
- 支持流程图保存 / 加载，复用已搭建的工作流
- 暗色主题 UI，支持 Windows 深色模式

### 图像模块
| 节点 | 说明 |
|------|------|
| **摄像头** | 实时摄像头采集，独立子线程抓帧，支持设备枚举、单帧 / 连续触发 |
| **图像列表** | 批量加载文件夹图像，逐张送入下游节点 |
| **显示图像** | 接收图像数据并实时显示 |

### 机器学习模块
| 节点 | 说明 |
|------|------|
| **模型预测** | 加载 TorchScript / ONNX 模型，对输入图像执行推理，绘制检测框与标签 |
| **模型训练** | 选择数据集与标注，配置训练参数，启动 YOLO 训练并实时显示 Loss 曲线 |

### 预训练模型
- **手势识别** — 34 类手势检测（基于 Hagrid 数据集训练）
- **人脸识别** — 人脸检测

### 图像标注工具（marker）
- 画布式标注界面，支持矩形、多边形、点、旋转矩形（OBB）标注
- 标注数据存入 SQLite 数据库，支持导入 / 导出
- 可将标注结果直接送入训练流程

### 训练引擎（trainer）
- YOLOv4-tiny + CSPdarknet53-tiny 网络结构（LibTorch C++ 实现）
- 数据增强（Augmentations）：随机裁剪、缩放、色彩扰动等
- 训练对话框实时绘制训练 / 验证 Loss 曲线（QCustomPlot）
- 训练完成自动保存最优权重

### 推理引擎
- `yolo_detect` — 基于 LibTorch + TorchScript 的 YOLO 推理，支持 NMS、letterbox、可选 CUDA 加速
- `dnn_yolo` — 基于 OpenCV DNN 的 ONNX 推理（编译宏 `USE_CVDNN` 切换）

---

## 技术栈

| 组件 | 版本 / 说明 |
|------|------------|
| Qt | 5.12.12（Widgets / SQL / PrintSupport） |
| QtNodes | 节点编辑器库（同级目录 `../QNodes`） |
| OpenCV | 5.0.0（core / imgcodecs / imgproc / highgui / dnn / freetype / videoio） |
| LibTorch | 2.0.1 CPU（c10 / torch_cpu） |
| QCustomPlot | 训练 Loss 曲线绘制 |
| SQLite | Qt SQL 模块，标注数据存储 |
| 编译器 | MSVC 2017 64bit，C++17 |

---

## 工程结构

```
AIFlow/
├── AIFlow.pro              # qmake 工程文件
├── main.cpp                # 程序入口，深色模式 / QSS 样式加载
├── mainwindow.{h,cpp,ui}   # 主窗口，节点注册与场景管理
├── darktreestyle.{h,cpp}   # 树形控件自定义样式
├── resource.qrc            # Qt 资源文件（图标 / 样式表）
├── VisionFlow.rc           # Windows 应用图标资源
├── msvc_make.bat           # jom 构建辅助脚本
│
├── nodes/                  # 节点数据类型（NodeData）
│   ├── node_image.h        #   图像数据
│   ├── node_net.h          #   模型数据
│   ├── node_dataset.h      #   训练数据集数据
│   └── node_add_image.h    #   附加图像数据
│
├── models/                 # 节点模型实现（NodeDelegateModel）
│   ├── model_camera_reader.*   # 摄像头节点
│   ├── model_image_reader.*    # 图像列表节点
│   ├── model_image_show.*      # 显示图像节点
│   ├── model_onnx_reader.*     # 预训练模型节点
│   ├── model_net_predict.*     # 模型预测节点
│   └── model_net_train.*       # 模型训练节点
│
├── marker/                 # 图像标注模块
│   ├── canvas.* / canvasview.* / canvasdlg.*   # 标注画布
│   ├── rectshape.* / polyshape.* / pointshape.* / rotatedrectshape.*  # 标注形状
│   ├── handleitem.* / obbhandle.*              # 控制手柄
│   ├── exporter.*           # 标注数据导入导出（SQLite）
│   └── baseshape.* / shapeutil.h
│
├── trainer/                # 训练引擎
│   ├── Detector.*          # 训练 / 推理封装（QObject，信号槽）
│   ├── DetDataset.*        # 检测数据集加载
│   ├── traindlg.*          # 训练对话框（Loss 曲线）
│   ├── qcustomplot.*       # 绘图库
│   ├── net/                # 网络结构定义
│   │   ├── yolo4_tiny.*
│   │   ├── CSPdarknet53_tiny.*
│   │   └── yolo_training.*
│   └── net_utils/          # 训练工具
│       ├── Augmentations.*  # 数据增强
│       ├── netutil.*
│       └── readfile.h / _dirent.h
│
├── yolo_detect.{h,cpp}     # TorchScript YOLO 推理引擎
├── dnn_yolo.{h,cpp}        # OpenCV DNN ONNX 推理引擎（备选）
├── utils/                  # 通用工具函数
└── res/                    # 图标、QSS 样式表
```

---

## 编译构建

### 环境要求

1. **Qt 5.12.12** + MSVC 2017 64bit 工具链
2. **OpenCV 5.0.0**，默认路径 `D:/env/opencv/opencv500/build/install`
3. **LibTorch 2.0.1 CPU**，默认路径 `D:/env/libtorch201cpu/libtorch`
4. **QNodes 节点库**，需先编译同级目录 `../QNodes`，输出至 `../output/QNodes/`

### 编译步骤

```bash
# 1. 编译 QNodes 依赖库（首次）
cd ../QNodes
qmake QNodes.pro
jom release      # 或 nmake / mingw32-make

# 2. 编译 AIFlow
cd ../AIFlow
qmake AIFlow.pro
jom release
```

### 路径配置

若你的 OpenCV / LibTorch 安装路径不同，修改 `AIFlow.pro` 中的：

```pro
OPENCV_DIR = D:/env/opencv/opencv500/build/install
INCLUDEPATH += D:/env/libtorch201cpu/libtorch/include
LIBS += -LD:/env/libtorch201cpu/libtorch/lib
```

### 编译选项

| 宏 | 作用 |
|----|------|
| `USE_CVDNN` | 使用 OpenCV DNN + ONNX 推理（默认使用 LibTorch + TorchScript） |
| `CV_CUDA` | 启用 OpenCV CUDA 加速（需 CUDA 版 OpenCV） |
| `NODE_EDITOR_SHARED` | 以动态库方式链接 QtNodes |

---

## 使用方法

### 基本操作
1. 启动程序，左侧「可用节点」树展示所有节点
2. 双击节点或拖拽到画布创建
3. 拖动节点端口之间的连线建立数据流
4. 顶部工具栏切换「移动模式 / 框选模式」

### 示例：摄像头 + 手势识别
```
[摄像头] ──图像──> [手势识别] ──图像──> [显示图像]
```
1. 创建「摄像头」节点，选择设备并启动采集
2. 创建「手势识别」节点
3. 创建「显示图像」节点
4. 连线：摄像头输出 → 手势识别输入 → 显示图像输入

### 示例：自定义模型训练
1. 使用标注工具对图像进行标注
2. 创建「模型训练」节点，选择数据集目录
3. 配置 epoch / batch_size / learning_rate
4. 点击训练，在训练对话框中观察 Loss 曲线
5. 训练完成后用「模型预测」节点加载生成的 TorchScript 模型进行推理

---

## 数据流类型

节点之间通过以下数据类型传递：

| 类型 ID | 显示名 | 说明 |
|---------|--------|------|
| `图像` | 图像 | `cv::Mat` 图像数据 |
| `模型` | 模型 | 模型路径 + 类别列表 |
| `训练数据集` | 训练数据集 | 数据集目录路径 |

---

## 依赖关系

```
AIFlow
 ├── QtNodes (动态库, ../output/QNodes/)
 https://github.com/paceholder/nodeeditor
 ├── OpenCV 5.0.0
 https://github.com/opencv/opencv
 ├── LibTorch (CPU)
 https://pytorch.org/
 ├── 模型训练
 https://github.com/AllentDan/LibtorchDetection

```

---


