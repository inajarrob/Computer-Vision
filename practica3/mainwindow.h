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


using namespace cv;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;

    VideoCapture *cap;
    ImgViewer *visorS, *visorD;
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


public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void chooseImage();
    void houghMethod(Mat canny);
    std::vector<std::vector<float>> harrisMethod();
    void SegmentDetection();
};


#endif // MAINWINDOW_H
