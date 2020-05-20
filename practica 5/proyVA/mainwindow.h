#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

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
    ImgViewer *visorSI, *visorSD, *visorD, *visorTD;
    Mat grayImageI, grayImageD, destGrayImage, trueDispImage;
    bool winSelected;
    Rect imageWindow;

    Mat fixedPoints, cornersLeft, disparity;

    bool isColor = false;
    Mat borders; // canny
    int idReg;
    Mat imgRegiones;
    struct region{
        Point pInicio;
        int nPuntos;
        uchar gMedio;
        float dMedia;
        int totalFijos;
    };
    std::vector<region> listRegiones;
    Mat imgMask;
    int points;
    int grisMedio;
    cv::Vec3i rgb;
    std::vector<std::vector<float>> pdcha, pizda;
    std::vector<Point> puntosD, puntosI;
    Mat dispImg;


public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void loadImage();
    std::vector<std::vector<float>> corners(Mat img);
    void segmentation();
    void addListRegiones(Point actual);
    void iniciar_disparity();
    void compareCorners(std::vector<std::vector<float>> result1, std::vector<std::vector<float>> result2);
    void drawCorners();
    void disparityNonFixed();
    void load_disparityImg();
    void update_disparity(QPointF p, int w, int h);

};


#endif // MAINWINDOW_H
