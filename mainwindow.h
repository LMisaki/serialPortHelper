#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum algorithm {
        GRAY = 0,
        IPM = 1,    //逆透视变换
        SOBEL = 2,
        FULLOSTU = 3,
        ADAPTIVE_BIN = 4,   //自适应阈值
        SCALE = 5,
        EIGHT_NB = 6,   //八邻域
        MIGONG = 7
    };

    QImage* grayImage(QImage* image);
    QImage* IPMImage(QImage* image);
    QImage* sobelImage(QImage* image);
    QImage* fullOstuImage(QImage* image);
    QImage* adaptiveBINImage(QImage* image);
    QImage* scaleImage(QImage* image);
    QImage* eightNBImage(QImage* image);
    QImage* miGongImage(QImage* image);
    QImage* findLine_leftHand(QImage *image);
    QImage* findLine_rightHand(QImage *image);

    QImage* toGrayImage(QImage * image);
    QImage* algorithmProcess(QImage* image , int index);

    QLabel* getCurrentTabLabel();
    QImage* getCurrentTabLabelImage();

    void refreshImageInTab(QImage *image);

private slots:
    void on_openButton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_tabWidget_tabCloseRequested(int index);

    void on_currentTabCB_stateChanged(int arg1);

    void on_algorithmList_itemClicked(QListWidgetItem *item);

    void on_tabWidget_tabBarDoubleClicked(int index);

    void on_openFolderButton_clicked();

    void on_playButton_clicked();

    void onPlayTimeout();

    void pause();

    void on_playSlider_valueChanged(int value);

    void on_playSlider_sliderPressed();

    void on_playSlider_sliderReleased();

    void on_serialPortButton_clicked();

private:
    /* 前进方向定义：
     *   0
     * 3   1
     *   2
     */
    const int dir_front[4][2] = {{0,  -1},
            {1,  0},
            {0,  1},
            {-1, 0}};

    /*
     *  分别为 0/1/2/3种前进方向的左前方位置
     */
    const int dir_frontleft[4][2]= {{-1, -1},
            {1,  -1},
            {1,  1},
            {-1, 1}};

    /*
     *  分别为 0/1/2/3种前进方向的右前方位置
     */
    const int dir_frontright[4][2] = {{1,  -1},
            {1,  1},
            {-1, 1},
            {-1, -1}};



    Ui::MainWindow *ui;

    int threshold_seed;
    int image_index;    //图像索引

    bool onCurrentTab;
    bool playing;
    bool sliderPressing;


    QTimer* playTimer;
    QImage *mainImage;
    QStringList imageList;
    QMap<QWidget*, QVector<int>> operationRecord;

    QVector<QRgb> pixelData;

};
#endif // MAINWINDOW_H
