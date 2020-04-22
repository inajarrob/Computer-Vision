#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"

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
    imgRegiones.create(240, 320, CV_32SC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);
    if(!isColor)
        visorResize = visorResize = new ImgViewer(&destGrayImage, resizeWindow.imgResize);
    else
        visorResize = visorResize = new ImgViewer(&destColorImage, resizeWindow.imgResize);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    // --------------------------------------------------------------------------------------------------
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(loadImage()));
    connect(ui->resizeButton, SIGNAL(clicked()), this, SLOT(resizeImg()));
    connect(&resizeWindow, SIGNAL(signalResize()), this, SLOT(closeResize()));
    connect(resizeWindow.okButton, SIGNAL(clicked()), this, SLOT(closeResize()));
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


    //En este punto se debe incluir el código asociado con el procesamiento de cada captura

    // Hacer canny para obtención de bordes
    cv::Canny(grayImage, borders, 0, 255);

    imgRegiones.setTo(-1);
    listRegiones.clear();
    idReg = 0;
    cv::copyMakeBorder(borders, imgMask, 1, 1, 1, 1, 1, BORDER_CONSTANT);

    if(ui->bottomup->isChecked())
        segmentation();

    if(ui->Drawlimits->isChecked()){
        drawImage();
    }

    //Actualización de los visores

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green);

    visorS->update();
    visorD->update();
    if(ui->resizeButton->isChecked())
        visorResize->update();
}

void MainWindow::start_stop_capture(bool start)
{
    if(start){
        ui->captureButton->setText("Stop capture");
        width  = 0;
        height = 0;
    }else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);
        isColor = true;
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
        isColor = false;
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

void MainWindow::loadImage(){
        start_stop_capture(false);
        disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
        ui->captureButton->setChecked(false);
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        "Load an Image",
                                                        "/home/",
                                                        "Image Files (*.png *.jpg)");
        const std::string file = fileName.toStdString();

        if(!fileName.isEmpty()){
            cv::Mat img = cv::imread(file, IMREAD_COLOR);
            width = img.size().width;
            height = img.size().height;
            std::cout << "Width: " << width << " Height: " << height << std::endl;
            dest.create(height, width, CV_8UC1);
            cv::resize(img, img, Size(320,240));
            cvtColor(img, img, COLOR_BGR2RGB);
            img.copyTo(colorImage);
            cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);
            cvtColor(colorImage, destGrayImage, COLOR_RGB2GRAY);
        } else{
            start_stop_capture(true);
            ui->captureButton->setChecked(true);
        }
        connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::segmentation(){
    Rect minRect;
    Point actual;
    int lo = ui->maxDifference->value(); //1 == 0 ? 0 : 20;
    int up = ui->maxDifference->value();//1 == 0 ? 0 : 20;
    Mat imgSegmented;
    if(!isColor)
        grayImage.copyTo(imgSegmented);
    else
        colorImage.copyTo(imgSegmented);

    // Lanzamos el crecimiento de regiones
    for (int i = 0; i < imgSegmented.rows; i++) {
        for (int j = 0; j < imgSegmented.cols; j++) {
            if(imgRegiones.at<int>(i,j) == -1 and imgMask.at<uchar>(i+1,j+1) == 0){
                grisMedio = 0;
                rgb[0]=0;
                rgb[1]=0;
                rgb[2]=0;
                actual.x = j;
                actual.y = i;

                int flag = 4|  (1 << 8)| FLOODFILL_MASK_ONLY;
                if(!ui->floatingRange->isChecked())
                    flag = flag | FLOODFILL_FIXED_RANGE;

                cv::floodFill(imgSegmented, imgMask, actual, ui->maxDifference->value(), &minRect, Scalar(lo, lo, lo), Scalar(up, up, up), flag);
                puntos = 0;
                for (int x = 0; x < minRect.width; x++) {
                    for(int y = 0; y < minRect.height; y++){
                        int auxX = minRect.x + x;
                        int auxY = minRect.y + y;
                        if(imgRegiones.at<int>(auxY,auxX) == -1 and imgMask.at<uchar>(auxY+1, auxX+1) == 1){
                            imgRegiones.at<int>(auxY,auxX) = idReg;
                            puntos++;
                            rgb[0] += colorImage.at<Vec3b>(Point(auxX,auxY))[0];
                            rgb[1] += colorImage.at<Vec3b>(Point(auxX,auxY))[1];
                            rgb[2] += colorImage.at<Vec3b>(Point(auxX,auxY))[2];
                            grisMedio += grayImage.at<uchar>(Point(auxX,auxY));
                        }
                    }
                }
                addListRegiones(actual);
                idReg++;
            }
        }
    }

    // post-procesamiento
    if(!isColor){
        for (int i = 0; i < grayImage.rows; i++) {
            for (int j = 0; j < grayImage.cols; j++) {
                int min = 256;
                int regionAux = -999;
                if(imgRegiones.at<int>(i,j) == -1){
                    // comparamos con los vecinos
                    int value = grayImage.at<uchar>(i,j);
                    for (int v = i-1; v < i+2; v++) {
                        for (int v2 = j-1; v2 < j+2; v2++) {
                           if((grayImage.cols > v2) and (v2 > -1) and (grayImage.rows > v)  and (v > -1)
                                    and min > abs(value - grayImage.at<uchar>(v, v2))
                                    and imgRegiones.at<int>(v, v2) != -1){
                                min       = abs(value - grayImage.at<uchar>(v, v2));
                                regionAux = imgRegiones.at<int>(v, v2);
                            }
                        }
                    }
                    imgRegiones.at<int>(i,j) = regionAux;
                    listRegiones[regionAux].nPuntos++;
                }
            }
        }
    } else {
        for (int i = 0; i < colorImage.rows; i++) {
            for (int j = 0; j < colorImage.cols; j++) {
                int minR = 256;
                int minG = 256;
                int minB = 256;
                int regionAux = -999;
                if(imgRegiones.at<int>(i,j) == -1){
                    Vec3b value = colorImage.at<Vec3b>(i,j);
                    for (int v = i-1; v < i+2; v++) {
                        for (int v2 = j-1; v2 < j+2; v2++) {
                           Vec3b actual = colorImage.at<Vec3b>(v,v2);
                           if((colorImage.cols > v2) and (v2 > -1) and (colorImage.rows > v)  and (v > -1)
                                    and minR > abs(value[0] - actual[0])
                                    and minG > abs(value[1] - actual[1])
                                    and minB > abs(value[2] - actual[2])
                                    and imgRegiones.at<int>(v, v2) != -1){
                                minR       = abs(value[0] - actual[0]);
                                minG       = abs(value[1] - actual[1]);
                                minB       = abs(value[2] - actual[2]);
                                regionAux = imgRegiones.at<int>(v, v2);
                            }
                        }
                    }
                    imgRegiones.at<int>(i,j) = regionAux;
                    listRegiones[regionAux].nPuntos++;
                }
            }
        }
    }


    // Vemos las fronteras
        bool stop = false;
        for (int i = 0; i < imgSegmented.rows; i++) {
            for (int j = 0; j < imgSegmented.cols; j++) {
                int valueReg = imgRegiones.at<int>(i,j);
                stop = false;

                if(i > 0 and j > 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                    // vecino 1
                    if(valueReg != imgRegiones.at<int>(i-1, j-1) and i > 0 and j > 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 2
                    if(valueReg != imgRegiones.at<int>(i-1, j) and i > 0 and j >= 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 3
                    if(valueReg != imgRegiones.at<int>(i-1, j+1) and i > 0 and j >= 0 and i < imgRegiones.rows and j < imgRegiones.cols-1){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 4
                    if(valueReg != imgRegiones.at<int>(i, j-1) and i >= 0 and j > 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 5
                    if(valueReg != imgRegiones.at<int>(i, j+1) and i >= 0 and j >= 0 and i < imgRegiones.rows and j < imgRegiones.cols-1){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 6
                    if(valueReg != imgRegiones.at<int>(i+1, j-1) and i >= 0 and j > 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 7
                    if(valueReg != imgRegiones.at<int>(i+1, j) and i >= 0 and j > 0 and i < imgRegiones.rows and j < imgRegiones.cols){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                    // vecino 8
                    if(valueReg != imgRegiones.at<int>(i+1, j+1) and i >= 0 and j >= 0 and i < imgRegiones.rows and j < imgRegiones.cols-1){
                            listRegiones[imgRegiones.at<int>(i, j)].frontera.push_back(Point(j,i));
                            stop = true;
                            continue;
                    }
                }
            }
        }


        // pintamos los resultados
        for (int i = 0; i < imgRegiones.rows; i++) {
            for (int j = 0; j < imgRegiones.cols; j++) {
                if(listRegiones.size() > imgRegiones.at<int>(i,j)){
                        destGrayImage.at<uchar>(i,j) = listRegiones[imgRegiones.at<int>(i,j)].gMedio;
                        destColorImage.at<Vec3b>(i,j) = listRegiones[imgRegiones.at<int>(i,j)].rgbMedio;
                    }
            }
        }
}

void MainWindow::addListRegiones(Point actual){
    region aux;
    aux.pInicio = actual;
    aux.nPuntos = puntos;
    aux.gMedio  = grisMedio/puntos;
    std::vector<Point> list;
    aux.frontera = list;
    aux.rgbMedio[0] = rgb[0]/puntos;
    aux.rgbMedio[1] = rgb[1]/puntos;
    aux.rgbMedio[2] = rgb[2]/puntos;
    listRegiones.push_back(aux);
}

void MainWindow::drawImage(){
    for(int i=0; i < listRegiones.size(); i++){
        //std::cout << "Frontiers size:" <<  listRegiones.at(i).frontera.size() << std::endl;
        for (int j=0; j < listRegiones.at(i).frontera.size(); j++) {
            visorD->drawSquare(QPointF(listRegiones.at(i).frontera.at(j).x, listRegiones.at(i).frontera.at(j).y), 1, 1, Qt::blue);
            if(ui->resizeButton->isChecked())
                visorResize->drawSquare(QPointF((listRegiones.at(i).frontera.at(j).x*width/320), (listRegiones.at(i).frontera.at(j).y*height/240)), 3, 3, Qt::blue);
        }
    }
}

void MainWindow::resizeImg(){
    if (width != 0 and height != 0) {
        resizeWindow.setGeometry(0,0,width*1.25, height*1.25);
        resizeWindow.update();
        // ----------------------------------------
        delete visorResize;
        visorResize = new ImgViewer(width, height, resizeWindow.imgResize);
        if(!isColor)
            cv::resize(destGrayImage, dest, Size(width, height));
        else
            cv::resize(destColorImage, dest, Size(width, height));
        visorResize->setImage(&dest);
        visorResize->update();
        // -----------------------------------------
        resizeWindow.imgResize->setGeometry(0,0,width, height);
        resizeWindow.imgResize->update();
        // ------- button --------------------------
        resizeWindow.okButton->setGeometry(width*1.25/2, height*1.20, 89, 25);
        resizeWindow.okButton->update();
    } else {
        cv::resize(destGrayImage, dest, Size(640, 480));
        visorResize->setImage(&dest);
        visorResize->update();
    }
    resizeWindow.show();
}

void QresizeImg::closeEvent(QCloseEvent *event){
    emit signalResize();
}

void MainWindow::closeResize(){
    resizeWindow.close();
    ui->resizeButton->setChecked(false);
}
