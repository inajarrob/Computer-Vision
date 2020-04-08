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
    esquinas.create(240, 320, CV_8UC1);

    grayImage.setTo(0);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);
    visorResize = new ImgViewer(&destGrayImage, resizeWindow.imgResize);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    // --------------------------------------------------------------------------------------------------
    connect(ui->loadImage,SIGNAL(clicked()),this,SLOT(chooseImage()));
    connect(ui->resizeImage, SIGNAL(clicked()), this, SLOT(resizeImg()));
    connect(&resizeWindow, SIGNAL(signalResize()), this, SLOT(closeResize()));
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

    // CANNY
    Mat canny;
    cv::Canny(destGrayImage, canny, ui->minThreshold->value(), ui->maxThreshold->value());
    if(ui->showCanny->isChecked()){
        canny.copyTo(destGrayImage);
    } else {
        grayImage.copyTo(destGrayImage);
    }

    // Segments to 0
    esquinas.setTo(0);

    // HOUGH LINES
    houghMethod(canny);
    if(ui->showLines->isChecked()){
        for (int i = 0; i < lines.size(); i++){
            visorD->drawLine(QLine(QPoint(lines[i].begin.x, lines[i].begin.y), QPoint(lines[i].end.x, lines[i].end.y)), Qt::green);
            if(ui->resizeImage->isChecked())
                visorResize->drawLine(QLine(QPoint((lines[i].begin.x*width/320), (lines[i].begin.y*height/240)), QPoint((lines[i].end.x*width/320), (lines[i].end.y*height/240))), Qt::green);
        }

    }


    // ESQUINAS HARRIS
    std::vector<std::vector<float>> puntosHarris = harrisMethod();
    if(ui->showCorners->isChecked()){
        for(int i = 0; i < puntosHarris.size(); i++)
        {
            visorD->drawSquare(QPoint(puntosHarris[i][1],puntosHarris[i][0]),5,5, Qt::red, true);
            if(ui->resizeImage->isChecked())
                visorResize->drawSquare(QPoint((puntosHarris[i][1]*width/320),(puntosHarris[i][0])*height/240),5,5, Qt::red, true);
        }
    }


    // DETECCION SEGMENTOS
    for (int i = 0; i < puntosHarris.size(); i++) {
        esquinas.at<uchar>(puntosHarris[i][0], puntosHarris[i][1]) = 1;
    }

    if(ui->showSegments->isChecked()){
        listSegments.clear();
        SegmentDetection(canny, puntosHarris, ui->resizeImage->isChecked());
    }


    //Actualización de los visores

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();
    if(ui->resizeImage->isChecked())
        visorResize->update();

}

void MainWindow::start_stop_capture(bool start)
{
    if(start){
        ui->captureButton->setText("Stop capture");
        width = 0;
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
        visorResize->setImage(&dest);

    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
        visorResize->setImage(&dest);
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

void MainWindow::chooseImage(){
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

void MainWindow::houghMethod(Mat canny){
    std::vector<Vec2f> linesH;
    HoughLines(canny, linesH, ui->rho->value(), ui->theta->value(), ui->thresholdHough->value());
    std::vector<Point> pointXY;
    Mat result;
    lines.clear();
    for(int i = 0; i < linesH.size(); i++){
        double p = linesH[i][0];
        double o = linesH[i][1];
        Point pt1, pt2;
        pointXY.clear();
        int x1 = rint((p - 0*sin(o))/cos(o)); // y1 = 0
        int y1 = rint((p - 319*cos(o))/sin(o)); // x1 = 319
        int x2 = rint((p - 239*sin(o))/cos(o)); // y2 = 239
        int y2 = rint((p - 0*cos(o))/sin(o)); // x2 = 0

        // Sacamos puntos
        if(x1 >=0 and x1 < width and std::find(pointXY.begin(), pointXY.end(), Point(x1,0)) == pointXY.end()){
                        pt1.x=x1;
                        pt1.y=0;
                        pointXY.push_back(pt1);
                    }
        if(y1 >= 0 and y1 < height and std::find(pointXY.begin(), pointXY.end(), Point(width,y1)) == pointXY.end()){
                        pt1.x=width;
                        pt1.y=y1;
                        pointXY.push_back(pt1);
                    }
        if(x2 < width and x2 >= 0 and std::find(pointXY.begin(), pointXY.end(), Point(x2,239)) == pointXY.end()){
                        pt2.x = x2;
                        pt2.y = height;
                        pointXY.push_back(pt2);
                    }
         if(y2 >= 0 and y2 < height and std::find(pointXY.begin(), pointXY.end(), Point(0, y2)) == pointXY.end()){
                        pt2.x = 0;
                        pt2.y = y2;
                        pointXY.push_back(pt2);
                    }
         if (pointXY.size() == 2){
             line localLine;
             localLine.begin = pointXY[0];
             localLine.end   = pointXY[1];
             lines.push_back(localLine);
         }
      }
}

std::vector<std::vector<float>> MainWindow::harrisMethod(){
    Mat imgHarris;
    cornerHarris(grayImage, imgHarris, ui->blockSize->value(),3, ui->harrisfactorSpin->value());
    // Supresion del no máximo
    Mat sorted;
    std::vector<std::vector<float>> puntos;

    for( int i = 0; i < imgHarris.rows ; i++ )
    {
        for( int j = 0; j < imgHarris.cols; j++ )
        {
            if(imgHarris.at<float>(i,j) > ui->thresholdSpin->value())
            {
                std::vector<float> aux;
                float valor = imgHarris.at<float>(i,j);
                aux.push_back(i+0.);
                aux.push_back(j+0.);
                aux.push_back(valor);
                puntos.push_back(aux);
            }
        }
    }

    // Ordenamos el vector de puntos
    std::sort(puntos.begin(), puntos.end(), [](const std::vector<float>& a, const std::vector<float>& b) {
        return a[2] > b[2];
    });

    // Recorremos el vector ordenado
    for(int i = 0; i < puntos.size(); i++)
    {
        // miramos los vecinos con la distancia euclídea
        for(int j = i+1; j < puntos.size(); j++)
        {
            float result = sqrt(pow(puntos[i][0]-puntos[j][0],2)+ pow(puntos[i][1]-puntos[j][1],2));
            if (result <= 13/2.)
            {
                puntos.erase(puntos.begin()+j);
                j--;
            }
        }
    }
    return puntos;
}

void MainWindow::SegmentDetection(Mat canny, std::vector<std::vector<float>> harris, bool resize){
    Point e1, e2;
    for (int i = 0; i<lines.size(); i++) {
        int nesquinas = 0;
        int npuntos = 0;

        if(abs(lines[i].end.y-lines[i].begin.y) > abs(lines[i].end.x - lines[i].begin.x)){
            Point p1, p2;
            if(lines[i].begin.y<lines[i].end.y)
            {
                p1 = lines[i].begin;
                p2 = lines[i].end;
            }
            else
            {
                p2 = lines[i].begin;
                p1 = lines[i].end;

            }

            for (int y = p1.y; y < p2.y; y++) {
                int x = ((y-p1.y)*(p2.x - p1.x))/(p2.y-p1.y) + p1.x;
                bool found = false;

                for (int xp = (x-ui->widthSegment->value()/2); xp <= (x+ui->widthSegment->value()/2); xp++) {
                    if(xp>=0 && xp<320)
                    {
                        if(esquinas.at<uchar>(y, xp) == 1){
                            nesquinas++;
                            if(nesquinas == 1){
                                e1.x = xp;
                                e1.y = y;
                                npuntos = 0;
                            } else {
                                e2.x = xp;
                                e2.y = y;
                            }
                        }
                        if (found == false and canny.at<uchar>(y, xp) == 255){
                            npuntos++;
                            found = true;
                        }
                    }
                }

                if(nesquinas == 2){
                    if(((float)npuntos)/((float)(e2.y-e1.y)) > ui->pointRadio->value() ){
                        line aux;
                        aux.begin = e1;
                        aux.end = e2;
                        listSegments.push_back(aux);
                        visorD->drawLine(QLine(e1.x, e1.y, e2.x, e2.y), Qt::blue, 3);
                        if(resize)
                            visorResize->drawLine(QLine((e1.x*width/320), (e1.y*height/240), (e2.x*width/320), (e2.y*height/240)), Qt::blue, 3);
                    }
                    e1 = e2;
                    nesquinas = 1;
                    npuntos = 0;

                }
            }
        }
        else {
            // x2-x1 > y2-y1
            Point p1, p2;
            if(lines[i].begin.x < lines[i].end.x)
            {
                p1 = lines[i].begin;
                p2 = lines[i].end;
            }
            else
            {
                p2 = lines[i].begin;
                p1 = lines[i].end;

            }

            for (int x = p1.x; x < p2.x; x++) {
                int y = ((x-p1.x)*(p2.y - p1.y))/(p2.x-p1.x) + p1.y;
                bool found = false;

                for (int yp = (y-ui->widthSegment->value()/2); yp <= (y+ui->widthSegment->value()/2); yp++) {
                    if(yp >= 0 and yp < 240){
                        if(esquinas.at<uchar>(yp, x) == 1){
                            nesquinas++;
                            if(nesquinas == 1){
                                e1.x = x;
                                e1.y = yp;
                                npuntos = 0;
                            } else {
                                e2.x = x;
                                e2.y = yp;
                            }
                        }
                        if (found == false and canny.at<uchar>(yp, x) == 255){
                            npuntos++;
                            found = true;
                        }
                    }
                }
                if(nesquinas == 2){
                    if(((float)npuntos)/((float)(e2.x-e1.x)) >= ui->pointRadio->value()){
                        if(SegmentComprobation(e1, e2)){
                            visorD->drawLine(QLine(e1.x, e1.y, e2.x, e2.y), Qt::blue, 3);
                            if(resize)
                                visorResize->drawLine(QLine((e1.x*width/320), (e1.y*height/240), (e2.x*width/320), (e2.y*height/240)), Qt::blue, 3);
                        }
                    }
                    e1 = e2;
                    nesquinas = 1;
                    npuntos = 0;
                }
            }
        }
    }
}


bool MainWindow::SegmentComprobation(Point e1, Point e2){
    bool find = true;
    line aux;
    aux.begin = e1;
    aux.end = e2;

    for (std::list<line>::iterator it = listSegments.begin(); it != listSegments.end(); it++) {
        if((it->begin == e1 and it->end == e2) or (it->end == e1 and it->begin == e2)){
            find = false;
            break;
        }
    }
    if(find)
        listSegments.push_back(aux);
    return find;
}

void MainWindow::resizeImg(){
    if (width != 0 and height != 0) {
        resizeWindow.setGeometry(0,0,width, height);
        resizeWindow.update();
        // ----------------------------------------
        delete visorResize;
        visorResize = new ImgViewer(width, height, resizeWindow.imgResize);
        cv::resize(destGrayImage, dest, Size(width, height));
        visorResize->setImage(&dest);
        visorResize->update();
        // -----------------------------------------
        resizeWindow.imgResize->setGeometry(0,0,width, height);
        resizeWindow.imgResize->update();
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
    ui->resizeImage->click();
}
