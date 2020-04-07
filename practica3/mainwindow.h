#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <math.h>
#include <imgviewer.h>
#include <QtWidgets/QFileDialog>
#include <ui_resizeimg.h>

using namespace cv;

namespace Ui {
    class MainWindow;
}

class QresizeImg: public QDialog, public Ui::Dialog
{
    Q_OBJECT
public:
    QresizeImg(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;
    QresizeImg resizeWindow;

    VideoCapture *cap;
    ImgViewer *visorS, *visorD, *visorResize;
    Mat colorImage, grayImage;
    Mat destColorImage, destGrayImage;
    Mat esquinas;
    bool winSelected;
    Rect imageWindow;
    struct line{
        Point begin;
        Point end;
    };
    std::list<line> listSegments;
    std::vector<line> lines;
    int width, height;
    Mat dest;


public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void chooseImage();
    void houghMethod(Mat canny, int width, int height);
    std::vector<std::vector<float>> harrisMethod();
    void SegmentDetection(Mat canny, std::vector<std::vector<float>> harris);
    bool SegmentComprobation(Point b, Point e);
    void resizeImg();
};


#endif // MAINWINDOW_H
