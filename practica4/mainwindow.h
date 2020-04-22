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
#include <math.h>
#include <QtWidgets/QFileDialog>
#include <ui_resize.h>


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
    void closeEvent(QCloseEvent *event);
signals:
    void signalResize();
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
    bool winSelected;
    bool isColor = false;
    Rect imageWindow;
    Mat borders; // canny
    int idReg;
    Mat imgRegiones;
    struct region{
        Point pInicio;
        int nPuntos;
        uchar gMedio;
        cv::Vec3b rgbMedio;
        std::vector<Point> frontera;
    };
    std::vector<region> listRegiones;
    Mat imgMask;
    int puntos;
    int grisMedio;
    cv::Vec3i rgb;
    int width, height;
    Mat dest;

public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void loadImage();
    void segmentation();
    void addListRegiones(Point actual);
    void drawImage();
    void closeResize();
    void resizeImg();

};


#endif // MAINWINDOW_H
