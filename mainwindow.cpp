#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QPixmap>
#include <QImage>
#include <QDebug>
#include <QDirIterator>
#include <cmath>
#include <QCollator>
#include <QMessageBox>
#include <serialpoat.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //TODO：暂时用于显示串口助手
    serialPoat * sp = new serialPoat();
    sp->show();

    mainImage = new QImage("C:/Users/Administrator/Pictures/82754731_p0.jpg");  //默认显示图像

    ui->setupUi(this);


    ui->algorithmList->clear();
    QStringList strList;
    strList<<"灰度图像"<<"逆透视变换"<<"sobel算子"<<"全局大津法"<<"自适应阈值"<<"SCALE"<<"八邻域巡线"<<"迷宫法寻线";
    ui->algorithmList->addItems(strList);

    playTimer = new QTimer(this);
    playTimer->setInterval(100);    //设置播放FPS 10帧/s
    connect(playTimer, &QTimer::timeout, this, &MainWindow::onPlayTimeout);

    ui->tabWidget->setTabsClosable(true);

    /*进度条设置*/
    ui->playSlider->setTracking(true);  //启用进度条实时跟踪
    ui->playSlider->hide(); //默认隐藏播放进度条

}

MainWindow::~MainWindow()
{
    delete ui;
}

//选择图片
void MainWindow::on_openButton_clicked()
{
    imageList.clear();  //选择图片后，清空图集
    ui->playSlider->setMaximum(0); //播放进度条范围归0
    ui->playSlider->hide(); //选择图片时隐藏播放进度条

    //选择图像
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择一张图片"), "C:/Users/Administrator/Pictures", tr("Image Files (*.png *.jpg *.bmp)"));
    if(fileName.isEmpty()) return;

    mainImage->load(fileName);

    QLabel *label = nullptr;
    if(ui->tabWidget->count() == 0) {
        label = new QLabel();
        label->setGeometry(10,10,564,360); //TODO:设置label大小
        QWidget *widget = new QWidget();    //创建界面和标签
        QVBoxLayout *layout = new QVBoxLayout(widget);  //设置垂直分布
        layout->addWidget(label);
        ui->tabWidget->addTab(widget,"原始图像");
    }else{
        label = getCurrentTabLabel();
    }

    label->setPixmap(QPixmap::fromImage(*mainImage).scaled(mainImage->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));    //TODO：图像过大显示不完全

}

//选择图集
void MainWindow::on_openFolderButton_clicked()
{
    imageList.clear();  //清空原来的图集
    QString imageListPath = QFileDialog::getExistingDirectory(this, tr("选择图集"), "C:/Users/Administrator/Pictures");
    if(imageListPath.isEmpty()) return;

    QDirIterator it(imageListPath,QStringList() << "*.png" << "*.jpg" << "*.bmp", QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()){
        imageList << it.next();
    }

    QCollator collator;
    collator.setNumericMode(true);  //设置比较模式为数值模式
    std::sort(imageList.begin(), imageList.end(),   //重新排序文件
              [& collator](const QString & str1, const QString & str2)
    {
          return collator.compare(str1, str2) < 0;
    }
    );

    //设置播放进度条数值范围
    ui->playSlider->setMinimum(0);
    ui->playSlider->setValue(0);
    ui->playSlider->setMaximum(imageList.size()-1);
    ui->playSlider->show();
    image_index=0;  //重置图像索引

    mainImage->load(imageList[0]);  //加载默认主图

    QLabel *label = nullptr;
    if(ui->tabWidget->count() == 0) {
        label = new QLabel();
        label->setGeometry(10,10,564,360); //TODO:设置label大小
        QWidget *widget = new QWidget();    //创建界面和标签
        QVBoxLayout *layout = new QVBoxLayout(widget);  //设置垂直分布
        layout->addWidget(label);
        ui->tabWidget->addTab(widget,"原始图像");
    }else{
        label = getCurrentTabLabel();
    }

    label->setPixmap(QPixmap::fromImage(*mainImage).scaled(mainImage->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

//播放点击函数
void MainWindow::on_playButton_clicked()
{
    if(playing){
        pause();
    }else{
        if(imageList.size()==0) {
            QMessageBox::warning(this,tr("Warning!!"),tr("请选择图集"));
            return ;
        }

        playTimer->start(); // 每秒切换一次图像
        playing=true;   //播放标志位
    }
}

//播放时钟超时函数
void MainWindow::onPlayTimeout(){
    QImage *image = new QImage(imageList[image_index]);   //将被处理的图像
    QWidget *widget = ui->tabWidget->currentWidget();

    auto it = operationRecord.find(widget);
    QVector<int> vec =  it.value(); //图像将被处理的步骤
    for (int algorithm:vec) {
        image = algorithmProcess(image,algorithm);  //遍历所有步骤
        qDebug()<<"当前处理算法:"<<algorithm;
    }

    ui->playSlider->setValue(image_index);    //播放进度条更新

    refreshImageInTab(image);   //刷新显示

    image_index = (image_index + 1) % imageList.count();  //循环播放
    qDebug()<<"当前图像索引:"<<imageList[image_index];
}

/*停止播放*/
void MainWindow::pause()
{
    playTimer->stop(); // 停止播放
    playing=false;  //标志位
}

/*播放进度条拖拽*/
void MainWindow::on_playSlider_valueChanged(int value)
{
    image_index = value;    //更新图像索引
    if(!sliderPressing) return; //正在播放，忽略信号

    pause();    //停止播放

    QImage *image = new QImage(imageList[value]);   //将被处理的图像
    QWidget *widget = ui->tabWidget->currentWidget();

    auto it = operationRecord.find(widget);
    QVector<int> vec =  it.value(); //图像将被处理的步骤
    for (int algorithm:vec) {
        image = algorithmProcess(image,algorithm);  //遍历所有步骤
    }

    refreshImageInTab(image);   //刷新显示
}

//进度条按压
void MainWindow::on_playSlider_sliderPressed()
{
    sliderPressing= true;
}
//进度条松开
void MainWindow::on_playSlider_sliderReleased()
{
    sliderPressing= false;
}

//算法选择框点击
void MainWindow::on_algorithmList_itemClicked(QListWidgetItem *item)
{
    QLabel *imageLabel = new QLabel();
    QImage *showImage = nullptr;
    QWidget *widget = new QWidget();    //创建界面和标签

    if(!onCurrentTab){
        operationRecord.insert(widget,QVector<int>()<<ui->algorithmList->currentRow()); //新建标签时，添加操作记录
        qDebug()<<"新标签绑定操作记录："<<ui->algorithmList->currentRow();

        QVBoxLayout *layout = new QVBoxLayout(widget);  //设置垂直分布
        layout->addWidget(imageLabel);
    }else {
        auto it = operationRecord.find(ui->tabWidget->currentWidget()); //原标签操作时，读取该标签，并在原来的vector中新增操作记录
        if(it != operationRecord.end()){
            it.value()<<ui->algorithmList->currentRow();
            qDebug()<<"原标签添加操作记录："<<ui->algorithmList->currentRow();
        }

        imageLabel = getCurrentTabLabel();
    }
    imageLabel->setGeometry(10,10,564,360); //TODO:设置label大小

    int index = ui->algorithmList->currentRow();
    showImage = algorithmProcess(nullptr,index);    //传入图像空指针时，默认显示主图

    switch(index){
    case GRAY:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"灰度图像");
        break;
    case IPM:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"逆透视变换");
        break;
    case SOBEL:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"Sobel边缘检测");
        break;
    case FULLOSTU:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"全局大津法");
        break;
    case ADAPTIVE_BIN:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"自适应阈值");
        break;
    case SCALE:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"scale");
        break;
    case EIGHT_NB:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"八邻域巡线");
        break;
    case MIGONG:
        if(!onCurrentTab) ui->tabWidget->addTab(widget,"迷宫法巡线");
        break;
    }
    imageLabel->setPixmap(QPixmap::fromImage(*showImage).scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); //保持缩放比例
    if(!onCurrentTab) ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);   //跳转到新的tab

}

//灰度图像
QImage* MainWindow::grayImage(QImage* img){
    QImage *image = nullptr;
    image = img != nullptr ? img : getCurrentTabLabelImage();
    image = toGrayImage(image);
    return image;
}

//逆透视变换
//TODO 能用，但参数需要调整，
QImage* MainWindow::IPMImage(QImage* img){

    QImage *preImage = nullptr; //原始图像，由于图像大小和比例不同，需要缩放
    preImage = img != nullptr ? img : getCurrentTabLabelImage();

    QImage *image = new QImage();
    if(preImage->height()!=188 || preImage->width()!=120) { //TODO：用常量替换数字
        *image =preImage->scaled(188,120,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);   //缩放至需要大小
    }

    int MT9V03X_H = image->height();   //懒得改原来的代码了，就这么命名了
    int MT9V03X_W = image->width();

    QImage *out_image = new QImage(*image); //输出图像

    //变换系数矩阵
    double change_un_Mat[3][3] ={{0.563593,-0.469635,15.155691},{0.006443,0.082527,5.857818},{-0.000141,-0.004577,0.692627}};

    //这里循环用的总钻风的数据大小188 * 120 而不是110 114（不全）
    for (int i = 0; i < MT9V03X_W ;i++) {
        for (int j = 0; j < MT9V03X_H ;j++) {
            int local_x = (int) ((change_un_Mat[0][0] * i
                    + change_un_Mat[0][1] * j + change_un_Mat[0][2])
                    / (change_un_Mat[2][0] * i + change_un_Mat[2][1] * j
                            + change_un_Mat[2][2]));
            int local_y = (int) ((change_un_Mat[1][0] * i
                    + change_un_Mat[1][1] * j + change_un_Mat[1][2])
                    / (change_un_Mat[2][0] * i + change_un_Mat[2][1] * j
                            + change_un_Mat[2][2]));
            //qDebug() << "IPM坐标 x y:" << i << j << "local x y: " << local_x << local_y;
            if (local_x >= 0&& local_y >= 0 && local_y < MT9V03X_H && local_x < MT9V03X_W){
                out_image->setPixelColor(i, j, image->pixelColor(local_x, local_y));
            }
            else {
                out_image->setPixelColor(i, j, QColor(255, 255, 255));
            }
        }
    }

    return out_image;
}

//Sobel算子边缘提取 源自B站“电个灯再睡觉”
//优化1：对绝对值函数优化
//速度较快的绝对值方法，来源于OPENCV源码'CV_IABS'定义 优点: 1.避免了使用abs函数进行跳转浪费时间 2.使用按位异或运算取绝对值，比直接相乘更快
#define FastABS(a) (((a) ^ ((a) < 0 ? -1 : 0)) - ((a) < 0 ? -1 : 0))
QImage* MainWindow::sobelImage(QImage* image){
    QImage *input = nullptr;
    input = image != nullptr ? image : getCurrentTabLabelImage();

    /* 卷积核大小 */
    short KERNEL_SIZE = 3;
    short xStart = KERNEL_SIZE / 2; //列卷积起始位置
    short xEnd = input->width() - KERNEL_SIZE / 2; //列卷积结束位置
    short yStart = KERNEL_SIZE / 2; //行卷积起始位置
    short yEnd = input->height() - KERNEL_SIZE / 2; //行卷积结束位置
    short i, j;
    short temp[2];

    //补边框黑线
    //ImageProcessDrawRect(output);

    //将灰度图的QImage转换成数组形式 此处不使用short使用unsigned char是为了便于与单片机移植代码对照
    unsigned char** imageIn = new unsigned char*[input->height()]; //输入数据
    for(int i = 0; i < input->height(); i++) {
        imageIn[i] = new unsigned char[input->width()];
    }
    //unsigned char** imageOut = new unsigned char*[input->height()];  //输出数据
    for(int i = 0; i < input->height(); i++) {
        imageIn[i] = new unsigned char[input->width()];
    }

    //移植到单片机时直接用拷贝函数
    for(int y = 0; y < input->height(); y++){
        for(int x = 0; x < input->width(); x++){
            imageIn[y][x] = (unsigned char)input->pixelColor(x, y).red(); //灰度图RGB三个通道数值相同
        }
    }

    //卷积遍历
    for (i = yStart; i < yEnd; i++)
    {
        for (j = xStart; j < xEnd; j++)
        {
            /* 计算y方向方向梯度幅值  */                                                        //  y->
            temp[0] = -(short) imageIn[i - 1][j - 1] + (short) imageIn[i - 1][j + 1] +      //x  {{-1, 0, 1},
            ( -((short) imageIn[i][j - 1]) + (short) imageIn[i][j + 1]) * 2                 //|   {-2, 0, 2},
            - (short) imageIn[i + 1][j - 1] + (short) imageIn[i + 1][j + 1];                //v   {-1, 0, 1}};

            /* 计算x方向方向梯度幅值  */
            temp[1] = -(short) imageIn[i - 1][j - 1] + (short) imageIn[i + 1][j - 1] +     //{{-1, -2, -1},
            ( -(short) imageIn[i - 1][j] + (short) imageIn[i + 1][j]) * 2                  // { 0,  0,  0},
            - (short) imageIn[i - 1][j + 1] + (short) imageIn[i + 1][j + 1];               // { 1,  2,  1}};

            temp[0] = FastABS(temp[0]);
            temp[1] = FastABS(temp[1]);

            //这里取得是两个方向的梯度幅度的最大值 而不是书上的两者绝对值之和？
            /* 找出梯度幅值最大值  */
            if (temp[0] < temp[1])
                temp[0] = temp[1];

            //关于阈值这里使用的是固定阈值 而书中使用的是全图梯度幅度最大之的百分比？
            //这里是否是考虑到赛道是平行线 x方向和y方向的梯度赋值一个会远大于另一个
            //后面实验一下两者相加的效果
            if (temp[0] > 60) input->setPixelColor(j, i, Qt::black);
            else    input->setPixelColor(j, i, Qt::white);
        }
    }

    //补边框黑线
    //ImageProcessDrawRect(output);

    return input;
}
QImage* MainWindow::fullOstuImage(QImage* image){
    QImage *image_copy = image;
    image_copy = image != nullptr ? image : getCurrentTabLabelImage();

    int MT9V03X_H = image_copy->height();   //懒得改原来的代码了，就这么命名了
    int MT9V03X_W = image_copy->width();

    int pixCount[256]={0};   //256 级灰度
    float sum_w=0;  //灰度级点数和
    int sum_gray=0;

    float pixPro[256]={0};    //灰度分布概率
    float w0=0,u0=0,u=0;  //代表设置阈值后，目标点数占全图的比例 和 目标的平均灰度值 和 图像总的平均灰度值;
    float g=0;  //最后求出的最大方差


    //memcpy(image_copy, mt9v03x_image, sizeof(mt9v03x_image));   //复制图像信息到副本，在副本中操作
    int threshold=0;
    //if(threshold==0){   //开机时计算一遍，去掉后一直算，动态全局大津法
    for(int i=0;i<MT9V03X_W;i++){   //遍历全图，画出灰度直方图
        for(int j=0;j<MT9V03X_H;j++){
            pixCount[qGray(image_copy->pixel(i,j))]++;
            u+=qGray(image_copy->pixel(i,j))*1.0/(MT9V03X_H*MT9V03X_W);
        }
    }

    for(int i=0;i<256;i++){ //计算w0 u0同时，直接计算阈值，优化后同时进行
        float g_temp=0;
        pixPro[i]=pixCount[i]*1.0/(MT9V03X_H*MT9V03X_W);   //灰度分布概率
        sum_gray+=pixCount[i]*i;
        sum_w+=pixCount[i];

        if(sum_w!=0) u0=sum_gray/sum_w;
        w0+=pixPro[i];

        if(w0 > 1-1e-5) break;  //防止出现0.99999/(1-0.99999)导致的阈值每次都是250+
        g_temp=w0/(1-w0)*pow((u0-u),2);
        if(g_temp>g){
            g=g_temp;
            threshold=i;
        }
    }

    if(threshold == 0)return image_copy;    //当阈值为0时，说明本身就是灰度图，直接返回

    //}else{  //固定阈值时使用，可以去掉else
    for(int i=0;i<MT9V03X_W;i++){   //遍历全图，同时计算阈值和方差
        for(int j=0;j<MT9V03X_H;j++){
            int gray = qGray(image_copy->pixel(i,j));
            QColor setPix = gray<threshold?Qt::black:Qt::white;
            image_copy->setPixelColor(i,j,setPix);
        }
    }
    //}
    return image_copy;
}
QImage* MainWindow::adaptiveBINImage(QImage* image){
    QImage *preImage = new QImage(*mainImage);
    return preImage;
}
QImage* MainWindow::scaleImage(QImage* image){
    QImage *preImage = new QImage(*mainImage);
    return preImage;
}
QImage* MainWindow::eightNBImage(QImage* img){

    // 8邻域
    const int neighbors_l[8][2] = { { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 },
                                 { -1, 0 }, { -1, 1 }, { 0, 1 }, {1, 1} };

    const int neighbors_r[8][2] = { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 },
                                 { 1, 0 }, { 1, 1 }, { 0, 1 }, {-1, 1} };

    QImage *image = nullptr;
    image = img != nullptr ? img : getCurrentTabLabelImage();

    int ROW =image->width(),COL = image->height();
    threshold_seed =150;
    int x=ROW/2,y=COL-COL/5;  //设置种子的起始点，在中间偏下位置，太低会经常找不到边界起始点
    for (; x > 0; x--) if (qGray(image->pixel(x-1,y))< threshold_seed) break;   //从（94，12）开始向左寻找种子，具体开始位置需要更具装车上的位置决定

    int seed[2]={x,y};
    int temp =0;
    while(++temp<150 && 1 < seed[0] && seed[0] < ROW - 2 && 1 < seed[1] && seed[1] < COL - 2){  //TODO 优化边界条件
        int point[2];
        int i=0;    //TODO 优化边界条件
        for(i=0;i<8;i++){
            point[0] = seed[0]+neighbors_l[i][0];
            point[1] = seed[1]+neighbors_l[i][1];
            int color0 =qGray(image->pixel(point[0]-1,point[1]));
            int color1 =qGray(image->pixel(point[0],point[1]-1));
            int color2 =qGray(image->pixel(point[0]+1,point[1]));
            int color3 =qGray(image->pixel(point[0],point[1]+1));
            if(((color0==0 && color2 != 0) || (color1 == 0 && color3 != 0) || (color3 == 0 && color1 != 0)) && qGray(image->pixel(point[0],point[1])) ==255){
                image->setPixelColor(point[0],point[1],Qt::red);
                qDebug()<<"当前位置:"<<point[0]<<","<<point[1];
                seed[0]=point[0];
                seed[1]=point[1];
                break;
            }
        }
        if(i==8) break;
    }

    x=ROW/2,y=COL-COL/5;  //设置种子的起始点，在中间偏下位置，太低会经常找不到边界起始点
    for (; x < ROW-1; x++) if (qGray(image->pixel(x+1,y))< threshold_seed) break;

    seed[0]=x,seed[1]=y;
    temp =0;
    while(++temp<150 && 1 < seed[0] && seed[0] < ROW - 2 && 1 < seed[1] && seed[1] < COL - 2){  //TODO 优化边界条件
        int point[2];
        int i=0;    //TODO 优化边界条件
        for(i=0;i<8;i++){
            point[0] = seed[0]+neighbors_r[i][0];
            point[1] = seed[1]+neighbors_r[i][1];
            int color0 =qGray(image->pixel(point[0]-1,point[1]));//左
            int color1 =qGray(image->pixel(point[0],point[1]-1));//上
            int color2 =qGray(image->pixel(point[0]+1,point[1]));//右
            int color3 =qGray(image->pixel(point[0],point[1]+1));//下
            if(((color2==0 && color0 != 0) || (color1 == 0 && color3 != 0) || (color3 == 0 && color1 != 0)) && qGray(image->pixel(point[0],point[1])) ==255){
                image->setPixelColor(point[0],point[1],Qt::green);
                qDebug()<<"当前位置:"<<point[0]<<","<<point[1];
                seed[0]=point[0];
                seed[1]=point[1];
                break;
            }
        }
        if(i==8) break;
    }

    return image;
}
QImage* MainWindow::miGongImage(QImage* image){
    QImage *mt9v03x_image = nullptr;
    mt9v03x_image = image != nullptr ? image : getCurrentTabLabelImage();
    findLine_leftHand(mt9v03x_image);
    return findLine_rightHand(mt9v03x_image);
}

QImage *MainWindow::findLine_leftHand(QImage *mt9v03x_image){
    int ROW =mt9v03x_image->width(),COL = mt9v03x_image->height();

    threshold_seed =150;

    int x=ROW/2,y=COL-COL/5;  //设置种子的起始点，在中间偏下位置，太低会经常找不到边界起始点
    for (; x > 0; x--) if (qGray(mt9v03x_image->pixel(x-1,y))< threshold_seed) break;   //从（94，12）开始向左寻找种子，具体开始位置需要更具装车上的位置决定

    int block_size=3; //局部均值范围3*3
    int half = block_size / 2;  //初二后变为-1~+1
    int dir=0,turn=0,step=0;

    //跳出循环条件：x和y越界（全部跑完），限制边界访问，turn>=4(陷入死循环，全是黑块),
    while(step < COL*2 && half < x && x < ROW - half - 1 && half < y && y < COL - half - 1 && turn < 4){

        int threshold = 0;
        for (int dy = -half; dy <= half; dy++) {
            for (int dx = -half; dx <= half; dx++) {
                if(x+dx>=0 && x+dx<ROW && y+dy>=0 && y+dy<COL) threshold += qGray(mt9v03x_image->pixel(x+dx,y+dy));   //3*3范围内求灰度和
            }
        }
        threshold /= block_size * block_size;   //求出范围内的灰度均值
        threshold -= 3; //减去一个固定值，范围在2-5之间最佳（当周围灰度相近时，只靠均值容易误判）

        int front_value = qGray(mt9v03x_image->pixel(x+dir_front[dir][0],y+dir_front[dir][1]));  //求出前方的阈值
        int frontLeft_value =qGray(mt9v03x_image->pixel(x+dir_frontleft[dir][0],y+dir_frontleft[dir][1]));   //求出左前方的阈值

        if(front_value<threshold){  //前方是黑色,路不通
            dir=(dir+1)%4;  //转向
            turn++; //到4次说明死循环，跳出
        }else if(frontLeft_value<threshold){    //左前方是黑色，说明是直道，往前走
            x+=dir_front[dir][0];   //往前走
            y+=dir_front[dir][1];
            mt9v03x_image->setPixelColor(x,y,Qt::red);
//            pts[step][0] = x;   //记录下位置
//            pts[step][1] = y;
            step++;
            turn=0;
        }else{  //前方和左前方都不是黑色，说明是左拐，直接走到左前方
            x += dir_frontleft[dir][0];     //往左前方走
            y += dir_frontleft[dir][1];
            mt9v03x_image->setPixelColor(x,y,Qt::red);
//            pts[step][0] = x;   //记录位置
//            pts[step][1] = y;
            dir = (dir + 3) % 4;    //左拐一次==按顺序拐3次
            step++;
            turn = 0;
        }
    }
    //*num=step;  //将累计的步数，也就是边界数量更新，减小运算
    return mt9v03x_image;
}

QImage *MainWindow::findLine_rightHand(QImage *mt9v03x_image){

    int ROW =mt9v03x_image->width(),COL = mt9v03x_image->height();

    threshold_seed =150;

    int x=ROW/2,y=COL-COL/5;  //设置种子的起始点，在中间偏下位置，太低会经常找不到边界起始点
    for (; x < ROW-1; x++) if (qGray(mt9v03x_image->pixel(x+1,y))< threshold_seed) break;

    int block_size=3;
    int half = block_size / 2;
    int dir=0,turn=0,step=0;

    while(step < COL*2 && half < x && x < ROW - half - 1 && half < y && y < COL - half - 1 && turn < 4){

        int threshold = 0;
        for (int dy = -half; dy <= half; dy++) {
            for (int dx = -half; dx <= half; dx++) {
                if(x+dx>=0 && x+dx<ROW && y+dy>=0 &&y+dy<COL) threshold += qGray(mt9v03x_image->pixel(x+dx,y+dy));
            }
        }
        threshold /= block_size * block_size;
        threshold -= 3;

        int front_value = qGray(mt9v03x_image->pixel(x+dir_front[dir][0],y+dir_front[dir][1]));
        int frontright_value =qGray(mt9v03x_image->pixel(x+dir_frontright[dir][0],y+dir_frontright[dir][1]));

        if (front_value < threshold) {
            dir = (dir + 3) % 4;
            turn++;
        } else if (frontright_value < threshold) {
            x += dir_front[dir][0];
            y += dir_front[dir][1];
//            pts[step][0] = x;
//            pts[step][1] = y;
            mt9v03x_image->setPixelColor(x,y,Qt::green);
            step++;
            turn = 0;
        } else {
            x += dir_frontright[dir][0];
            y += dir_frontright[dir][1];
            dir = (dir + 1) % 4;
            mt9v03x_image->setPixelColor(x,y,Qt::green);
//            pts[step][0] = x;
//            pts[step][1] = y;
            step++;
            turn = 0;
        }
    }
    return  mt9v03x_image;
}

//图像算法处理步骤列表
QImage *MainWindow::algorithmProcess(QImage* image , int index){
    QImage *processImage = nullptr;

    switch(index){
    case GRAY:
        processImage = grayImage(image);
        //qDebug()<<"灰度图像";
        break;
    case IPM:
        processImage = IPMImage(image);
        //qDebug()<<"逆透视变换";
        break;
    case SOBEL:
        processImage = sobelImage(image);
        //qDebug()<<"Sobel边缘检测";
        break;
    case FULLOSTU:
        processImage = fullOstuImage(image);
        //qDebug()<<"全局大津法";
        break;
    case ADAPTIVE_BIN:
        processImage = adaptiveBINImage(image);
        //qDebug()<<"自适应阈值";
        break;
    case SCALE:
        processImage = scaleImage(image);
        //qDebug()<<"scale";
        break;
    case EIGHT_NB:
        processImage = eightNBImage(image);
        //qDebug()<<"八邻域巡线";
        break;
    case MIGONG:
        processImage = miGongImage(image);
        //qDebug()<<"迷宫法巡线";
        break;
    }
    return processImage;
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    playTimer->stop(); // 停止播放
    qDebug()<<"当前标签："<<index;
}

//删除标签页
void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    // 获取要关闭的 widget
    QWidget *widgetToClose = ui->tabWidget->widget(index);

    // 从 tabWidget 中移除 widget
    ui->tabWidget->removeTab(index);

    // 删除 widget 上的子组件
    qDeleteAll(widgetToClose->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));


    // 最后删除 widget 本身
    delete widgetToClose;

    //删除该标签的操作记录
    auto it =operationRecord.find( ui->tabWidget->currentWidget());
    operationRecord.erase(it);
}

//双击删除标签页
void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    //调用删除标签
    on_tabWidget_tabCloseRequested(index);
}

//获取当前tab内的label
QLabel *MainWindow::getCurrentTabLabel(){
        QLabel *tabLabel = nullptr;
        auto tab = ui->tabWidget->widget(ui->tabWidget->currentIndex());
        if(qstrcmp( tab->metaObject()->className(),"QLabel") == 0) tabLabel = qobject_cast<QLabel *>(tab);  //如果tab内只有一个组件，则指向组件本身
        else{   //有多个组件，
            QList<QLabel *> allLabel = tab->findChildren<QLabel *>();
            if(allLabel.count()==1) tabLabel = allLabel[0];
        }
    return tabLabel;
}

//获取当前tab内label上的图像
QImage *MainWindow::getCurrentTabLabelImage(){
    QImage *preImage = nullptr;
    if(!onCurrentTab) preImage = new QImage(*mainImage);
    else {
        QLabel *tabLabel = nullptr;
        auto tab = ui->tabWidget->widget(ui->tabWidget->currentIndex());
        if(qstrcmp( tab->metaObject()->className(),"QLabel") == 0) tabLabel = qobject_cast<QLabel *>(tab);  //如果tab内只有一个组件，则指向组件本身
        else{   //有多个组件，
            QList<QLabel *> allLabel = tab->findChildren<QLabel *>();
            if(allLabel.count()==1) tabLabel = allLabel[0];
        }
        QPixmap pixmap = tabLabel->pixmap()->copy();
        preImage = new QImage(pixmap.toImage());
    }
    return preImage;
}

//原图转灰度图
QImage *MainWindow::toGrayImage(QImage* image){
    int width = image->width();
    int height = image->height();

    //    //灰度图数组
    //    int **pixelArray = new int* [height];
    //    for(int y=0;y<height;++y){
    //        pixelArray[y] = new int[width];
    //    }


    for(int y = 0; y<height; ++y){
        QRgb *pixel = reinterpret_cast<QRgb*>(image->scanLine(y));
        QRgb *end = pixel + width;
        for (;pixel != end;++pixel) {
            int gray = qGray(*pixel);
            *pixel = QColor(gray, gray, gray).rgb();
        }
    }
    return image;
    //    弃用，使用指针遍历数组转灰度图，直接原图修改
    //            int **temp;
    //    temp =pixelArray;
    //    for(int y = 0; y<image.height(); ++y){
    //        QRgb *pixel = reinterpret_cast<QRgb*>(image.scanLine(y));
    //        QRgb *end = pixel + image.width();
    //        int *temp = *pixelArray;
    //        for (;pixel != end;++pixel,temp++) {
    //            int gray = qGray(*pixel);
    //            //*pixel = QColor(gray, gray, gray).rgb();
    //            *temp= qRgb(gray, gray, gray);
    //        }
    //        pixelArray++;
    //    }
    //    pixelArray = temp;

    //    // 将灰度数组转换为 QImage
    //    for (int y = 0; y < height; ++y) {
    //        for (int x = 0; x < width; ++x) {
    //            grayImage.setPixel(x, y, pixelArray[y][x]);
    //        }
    //    }

    //    弃用，两for遍历赋值到新数组，改成指针访问
    //            for (int y=0;y<height;++y) {
    //        for (int x = 0;x < width;++x) {
    //            //pixelArray[y][x] = qGray(image.pixel(x, y));
    //            //pixelArray[y][x] = (image.pixelColor(x,y).red()+image.pixelColor(x,y).green()+image.pixelColor(x,y).blue())/3;
    //        }
    //    }

    //    for(int y = 0; y<image.height(); ++y){
    //        QRgb *pixel = reinterpret_cast<QRgb*>(image.scanLine(y));
    //        QRgb *end = pixel + image.width();
    //        for (int x=0;pixel != end;++pixel,++x) {
    //            int gray = qGray(*pixel);
    //            //*pixel = QColor(gray, gray, gray).rgb();
    //            pixelArray[y][x]= qRgb(gray, gray, gray);
    //        }
    //    }

    //    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
}

//检查 在当前页面处理 选项是否勾选
void MainWindow::on_currentTabCB_stateChanged(int arg1)
{
    onCurrentTab =arg1; //0==flase，1 / 2 ==true
}

//刷新当前tab内的图像显示
void MainWindow::refreshImageInTab(QImage *image){
    QLabel *label = getCurrentTabLabel();
    //label->setGeometry(10,10,188,120); //TODO:设置label大小
    label->setPixmap(QPixmap::fromImage(*image).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); //保持缩放比例

}

void MainWindow::on_serialPortButton_clicked()
{
     serialPoat * sp = new serialPoat();
     sp->show();
}
