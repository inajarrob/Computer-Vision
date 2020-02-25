#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    // --------------------------- Botones creados --------------------------------
    connect(ui->buttonLoad,SIGNAL(clicked(bool)),this,SLOT(chooseImage(bool)));
    connect(ui->buttonSave,SIGNAL(clicked()),this,SLOT(saveImage()));
    connect(ui->buttonCopy,SIGNAL(clicked()),this,SLOT(copyImage()));
    connect(ui->buttonResize, SIGNAL(clicked(bool)), this, SLOT(resizeImg()));
    connect(ui->buttonEnlarge, SIGNAL(clicked(bool)), this, SLOT(enlargeImg()));
    // ---------------------------------------------------------------------------
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    colorImage.release();
    grayImage.release();
    destColorImage.release();
    destGrayImage.release();
}

void MainWindow::compute()
{

    //Captura de imagen
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320,240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }

    if(ui->ButtonWarpZoom->isChecked()){
        warpZoomImg();
    }

    //En este punto se debe incluir el código asociado con el procesamiento de cada captura
    colorImage.copyTo(visibleOrigin);
    destColorImage.copyTo(visibleDest);
    std::vector<cv::Mat> vectorO(3), vectorD(3);
    split(visibleOrigin,vectorO);
    split(visibleDest, vectorD);

    if (!ui->checkBoxR->isChecked()){
       vectorO[0].setTo(0);
       vectorD[0].setTo(0);
    }

    if (!ui->checkBoxG->isChecked()){
       vectorO[1].setTo(0);
       vectorD[1].setTo(0);
    }

    if (!ui->checkBoxB->isChecked()){
       vectorO[2].setTo(0);
       vectorD[2].setTo(0);
    }
    merge(vectorO, visibleOrigin);
    merge(vectorD, visibleDest);

    //Actualización de los visores
    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();

}

void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&visibleOrigin);
        visorD->setImage(&visibleDest);
        colorImg = true;
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
        colorImg = false;
    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x+1;
        imageWindow.height = pEnd.y()-imageWindow.y+1;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::chooseImage(bool clicked){

    start_stop_capture(false);
    ui->captureButton->setChecked(false);
    std::cout << cap->isOpened() << std::endl;
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Load an Image",
                                                    "/home/",
                                                    "Image Files (*.png *.jpg)");
    const std::string file = fileName.toStdString();

    if(!fileName.isEmpty()){
        cv::Mat img = cv::imread(file, IMREAD_COLOR);
        cv::resize(img, img, Size(320,240));
        cvtColor(img, img, COLOR_BGR2RGB);
        img.copyTo(colorImage);
        cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);
        cvtColor(colorImage, destGrayImage, COLOR_RGB2GRAY);
        ui->colorButton->click();
    } else{
        start_stop_capture(true);
        ui->captureButton->setChecked(true);
    }
}

void MainWindow::saveImage(){
    Mat imgSave;

    if(colorImg){
        // si la imagen esta a color guardamos imagen a color
        cvtColor(destColorImage, imgSave, COLOR_RGB2BGR);

    } else{
        cvtColor(destGrayImage, imgSave, COLOR_GRAY2BGR);
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"),
                                                    "",
                                                    tr("Images (*.png)"));
    if (!fileName.isEmpty())
    {
      cv::imwrite(fileName.toStdString(), imgSave);
    }
}

void MainWindow::copyImage(){
    if(winSelected){
        int x = (320 - imageWindow.width)/2;
        int y = (240 - imageWindow.height)/2;

        Rect destWindow;
        destWindow.x = x;
        destWindow.y = y;
        destWindow.width  = imageWindow.width;
        destWindow.height = imageWindow.height;
        /******************COLOR********************/
        destColorImage.setTo(0);
        Mat winDest  = destColorImage(destWindow);
        Mat(colorImage, imageWindow).copyTo(winDest);
        /******************GRAY*********************/
        destGrayImage.setTo(0);
        Mat winDestGray = destGrayImage(destWindow);
        Mat(grayImage, imageWindow).copyTo(winDestGray);
    } else {
        colorImage.copyTo(destColorImage);
        grayImage.copyTo(destGrayImage);
    }
}

void MainWindow::resizeImg(){
    if(winSelected){
        cv::resize(colorImage(imageWindow), destColorImage, Size(320, 240));
        cv::resize(grayImage(imageWindow), destGrayImage, Size(320, 240));
    }
}


void MainWindow::enlargeImg(){
    if(winSelected){
        destColorImage.setTo(0);
        destGrayImage.setTo(0);

        float fx = 320./imageWindow.width;
        float fy = 240./imageWindow.height;
        int x,y;
        int w, h;
        cv::Mat color, gray;

        if (fx > fy){
            w  = rint(imageWindow.width*fy);
            h  = rint(imageWindow.height*fy);
            x = (320-w)/2;
            y = 0;

            color.create(h, w, CV_8UC3);
            gray.create(h, w, CV_8UC1);

            cv::resize(Mat(colorImage, imageWindow), color, Size(), fy, fy);
            cv::resize(Mat(grayImage, imageWindow), gray, Size(), fy, fy);
            color.copyTo(destColorImage(Rect(x,y, color.cols, color.rows)));
            gray.copyTo(destGrayImage(Rect(x,y, gray.cols, gray.rows)));
        } else {
            w  = rint(imageWindow.width*fx);
            h  = rint(imageWindow.height*fx);
            x = 0;
            y = (240-h)/2;

            color.create(h, w, CV_8UC3);
            gray.create(h,w, CV_8UC1);

            cv::resize(Mat(colorImage, imageWindow), color, Size(), fx, fx);
            cv::resize(Mat(grayImage, imageWindow), gray, Size(), fx, fx);
            color.copyTo(destColorImage(Rect(x,y,color.cols, color.rows)));
            gray.copyTo(destGrayImage(Rect(x,y, gray.cols, gray.rows)));
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------
void MainWindow::warpZoomImg(){
    if(ui->ButtonWarpZoom->isChecked()){
        int horizontal = ui->horizontalTrans->value();
        int vertical = ui->verticalTrans->value();
        float zoom = (ui->Zoom->value())*4/99. + 1;
        int dial = ui->Angle->value();
        float auxD = (dial/180.)*M_PI;
        Mat aux;


        Matx<float, 2, 3> mt(cos(auxD), sin(auxD), horizontal+160-160*cos(auxD)-120*sin(auxD), -sin(auxD), cos(auxD), vertical+120+160*sin(auxD)-120*cos(auxD));
        if(colorImg)
            warpAffine(colorImage, aux, mt, Size(320, 240));
        else
            warpAffine(grayImage, aux, mt, Size(320, 240));
        cv::resize(aux, aux, Size(), zoom, zoom);

        int col = aux.cols;
        int row = aux.rows;
        int x = rint((col-320)/2.);
        int y = rint((row-240)/2.);

        Mat winAux = aux(Rect(x, y, 320, 240));
        if(colorImg)
            winAux.copyTo(destColorImage);
        else
            winAux.copyTo(destGrayImage);
    }
}
