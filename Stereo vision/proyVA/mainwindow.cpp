#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    grayImageI.create(240,320,CV_8UC1);
    grayImageD.create(240,320,CV_8UC1);
    trueDispImage.create(240,320,CV_8UC1);
    destGrayImage.create(240,320,CV_8UC1);

    imgRegiones.create(240, 320, CV_32SC1);
    cornersLeft.create(240, 320, CV_32FC1);
    fixedPoints.create(240, 320, CV_8UC1);
    disparity.create(240, 320, CV_32FC1);

    visorSI = new ImgViewer(&grayImageI, ui->imageFrameS);
    visorSD = new ImgViewer(&grayImageD, ui->imageFrameD);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrame_3);
    visorTD = new ImgViewer(&trueDispImage, ui->imageFrame_4);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorSI,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorSI,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    // --------------------------------------------------------------------------------------------------
    connect(ui->loadImage,SIGNAL(clicked()),this,SLOT(loadImage()));
    connect(ui->disparity, SIGNAL(clicked()), this, SLOT(iniciar_disparity()));
    connect(ui->loadGround, SIGNAL(clicked()), this, SLOT(load_disparityImg()));
    connect(visorD, SIGNAL(windowSelected(QPointF, int, int)), this, SLOT(update_disparity(QPointF, int, int)));
    connect(ui->propagate, SIGNAL(clicked()), this, SLOT(propagateDisparity()));
    connect(visorD, SIGNAL(windowSelected(QPointF, int, int)), this, SLOT(update_3DPosition(QPointF p, int w, int h)));
    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorSI;
    delete visorSD;
    delete visorD;
    delete visorTD;
    grayImageD.release();
    grayImageI.release();
    trueDispImage.release();
    destGrayImage.release();
}

void MainWindow::compute()
{

    //Captura de imagen
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> grayImageI;
        cv::resize(grayImageI, grayImageI, Size(320,240));
        cvtColor(grayImageI, grayImageI, COLOR_BGR2GRAY);
    }


    //En este punto se debe incluir el código asociado con el procesamiento de cada captura
    if(ui->showCorners->isChecked()){
        drawCorners();
    }

    if(ui->propagate->isChecked()){
        propagateDisparity();
    }

    //Actualización de los visores


    if(winSelected)
        visorSI->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green);

    visorSI->update();
    visorSD->update();
    visorD->update();
    visorTD->update();
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
        ui->colorButton->setText("Color image");
        visorSD->setImage(&grayImageI);
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

    QStringList fileName = QFileDialog::getOpenFileNames(this,
                                                    "Load an Image",
                                                    "/home/",
                                                    "Image Files (*.png *.jpg)");
    if(fileName.size() > 1){
        const std::string file  = fileName[0].toStdString();
        const std::string file2 = fileName[1].toStdString();
        cv::Mat img = cv::imread(file, IMREAD_COLOR);
        cv::Mat img2 = cv::imread(file2, IMREAD_COLOR);
        cv::resize(img, img, Size(320,240));
        cv::resize(img2, img2, Size(320,240));
        cvtColor(img, grayImageI, COLOR_BGR2GRAY);
        cvtColor(img2, grayImageD, COLOR_BGR2GRAY);
    } else{
        start_stop_capture(true);
        ui->captureButton->setChecked(true);
    }
    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

std::vector<std::vector<float>> MainWindow::corners(Mat img){
    fixedPoints.setTo(0);
    disparity.setTo(0);

    Mat imgHarris;
    cornerHarris(img, imgHarris, 3, 3, 0.04);
    // Supresion del no máximo
    Mat sorted;
    std::vector<std::vector<float>> puntos;

    for( int i = 0; i < imgHarris.rows ; i++ )
    {
        for( int j = 0; j < imgHarris.cols; j++ )
        {
            if(imgHarris.at<float>(i,j) > 0.000001)
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

void MainWindow::compareCorners(std::vector<std::vector<float>> puntosIzda, std::vector<std::vector<float>> puntosDcha){
    // Pasamos a comprobar los puntos que obtenemos de la imagen izquierda con la imagen derecha
    Mat result;
    int w = 11;
    //std::vector<Point> puntos;
    Mat aux;
    grayImageI.copyTo(aux);
    aux.setTo(0);
    std::cout<< "initialize aux Mat: " << puntosDcha.size() << std::endl;
    for(int i = 0; i < puntosDcha.size(); i++){
        aux.at<uchar>(puntosDcha[i][0],puntosDcha[i][1]) = 1;
    }

    puntosD.clear();
    puntosI.clear();

    for(int i = 0; i < puntosIzda.size(); i++){
        float best = 0;
        Point bestP;

        for (int j = 0; j < puntosIzda[i][1]; j++) {
            Point pDcha(j, puntosIzda[i][0]); // el punto que hay que consultar de la imagen derecha es el que tiene columna j y la misma fila que la esquina izquierda

            if(aux.at<uchar>(pDcha)==1 and puntosIzda[i][1] >= w/2 and puntosIzda[i][0] >= w/2 and pDcha.x >= w/2 and pDcha.y >= w/2
               and puntosIzda[i][1]+w/2 < 320 and puntosIzda[i][0]+w/2 < 240 and pDcha.x+w/2 < 320 and pDcha.y+w/2 < 240){
                Mat left = grayImageI(Rect(puntosIzda[i][1]-w/2, puntosIzda[i][0]-w/2, w, w));
                Mat right = grayImageD(Rect(pDcha.x-w/2, pDcha.y-w/2, w, w)); // ventana centrada en la esquina de la derecha
                matchTemplate(right, left, result, TM_CCOEFF_NORMED);

                if(result.at<float>(0,0) > best){
                    best = result.at<float>(0,0);
                    bestP = pDcha;
                }
            }
        }
        if (best > 0.8 and (puntosIzda[i][1] - bestP.x) > 0 and compareOpposite(Point(puntosIzda[i][1], puntosIzda[i][0]), bestP, puntosIzda)){
            fixedPoints.at<uchar>(puntosIzda[i][0], puntosIzda[i][1]) = 1;
            disparity.at<float>(puntosIzda[i][0], puntosIzda[i][1]) = puntosIzda[i][1] - bestP.x;
            puntosD.push_back(bestP);
            puntosI.push_back(Point(puntosIzda[i][1], puntosIzda[i][0]));
        }
    }
}

// Pasamos a comprobar los puntos que obtenemos de la imagen derecha con la imagen izquierda
bool MainWindow::compareOpposite(Point pI, Point pD, std::vector<std::vector<float>> puntosIzda){
    std::cout << "punto izda -- dcha: " << pI << pD << std::endl;
    bool same = false;
    Mat result;
    int w = 11;
    Mat aux;
    float best = 0;
    Point bestP;
    grayImageI.copyTo(aux);
    aux.setTo(0);

    for(int i = 0; i < puntosIzda.size(); i++){
        aux.at<uchar>(puntosIzda[i][0],puntosIzda[i][1]) = 1;
    }

    for (int j = 0; j < aux.cols; j++) {
        if(aux.at<uchar>(pD.y,j)==1 and
           pD.x >= w/2 and pD.y >= w/2 and
           pD.x < 321 and pD.y < 241 and
           j+w/2 > 0 and j+w/2 < 320
           and (j-w/2) > 0 and (j-w/2) <= 320){
            Mat left = grayImageD(Rect(pD.x-w/2, pD.y-w/2, w, w));
            Mat right = grayImageI(Rect(j - w/2, pD.y -w/2, w, w));; // ventana centrada en la esquina de la derecha
            matchTemplate(right, left, result, TM_CCOEFF_NORMED);

            if(result.at<float>(0,0) > best){
                best = result.at<float>(0,0);
                bestP = Point(j, pD.y);
            }
        }
    }
    if (best > 0.8 and (bestP == pI)) {
        return true;
    }
    return false;
}

void MainWindow::segmentation(){
    Rect minRect;
    Point actual;
    int lo = 10;
    int up = 10;
    int flag = 4|  (1 << 8)| FLOODFILL_MASK_ONLY;

    cv::Canny(grayImageD, borders, 0, 255);
    imgRegiones.setTo(-1);
    listRegiones.clear();
    idReg = 0;
    cv::copyMakeBorder(borders, imgMask, 1, 1, 1, 1, 1, BORDER_CONSTANT);

    // Lanzamos el crecimiento de regiones
    std::cout << "crecimiento de regiones" << std::endl;
    for (int i = 0; i < grayImageD.rows; i++) {
        for (int j = 0; j < grayImageD.cols; j++) {
            std::cout << "i: "<< i << " j: " << j << std::endl;
            if(imgRegiones.at<int>(i,j) == -1 and imgMask.at<uchar>(i+1,j+1) == 0){
                std::cout << "hola" << std::endl;
                grisMedio = 0;
                actual.x = j;
                actual.y = i;
                cv::floodFill(grayImageD, imgMask, actual, lo, &minRect, Scalar(lo, lo, lo), Scalar(up, up, up), flag);
                std::cout << "despues floodFill" << std::endl;
                points = 0;
                for (int x = 0; x < minRect.width; x++) {
                    for(int y = 0; y < minRect.height; y++){
                        int auxX = minRect.x + x;
                        int auxY = minRect.y + y;
                        if(imgRegiones.at<int>(auxY,auxX) == -1 and imgMask.at<uchar>(auxY+1, auxX+1) == 1){
                            imgRegiones.at<int>(auxY,auxX) = idReg;
                            points++;
                            grisMedio += grayImageD.at<uchar>(Point(auxX,auxY));
                        }
                    }
                }
                std::cout << "añadimos una nueva region" << std::endl;
                addListRegiones(actual);
                idReg++;
            }
        }
    }
    std::cout << "post procesamiento" << std::endl;

    // post-procesamiento
    for (int i = 0; i < grayImageD.rows; i++) {
        for (int j = 0; j < grayImageD.cols; j++) {
            int min = 256;
            int regionAux = -999;
            if(imgRegiones.at<int>(i,j) == -1){
                // comparamos con los vecinos
                int value = grayImageD.at<uchar>(i,j);
                for (int v = i-1; v < i+2; v++) {
                    for (int v2 = j-1; v2 < j+2; v2++) {
                       if((grayImageD.cols > v2) and (v2 > -1) and (grayImageD.rows > v)  and (v > -1)
                                and min > abs(value - grayImageD.at<uchar>(v, v2))
                                and imgRegiones.at<int>(v, v2) != -1){
                            min       = abs(value - grayImageD.at<uchar>(v, v2));
                            regionAux = imgRegiones.at<int>(v, v2);
                        }
                    }
                }
                imgRegiones.at<int>(i,j) = regionAux;
                listRegiones[regionAux].nPuntos++;
            }
        }
    }

    // calculo disparity
    std::cout << "calculo de disparidad" << std::endl;
    for (int i = 0; i < imgRegiones.rows; i++) {
        for (int j = 0; j < imgRegiones.cols; j++) {
            if(fixedPoints.at<uchar>(i,j) == 1) {
                int numberR = imgRegiones.at<int>(i,j);
                listRegiones[numberR].dMedia += disparity.at<float>(i,j);
                listRegiones[numberR].totalFijos++;
            }
        }
    }

    for(int i = 0; i < listRegiones.size(); i++){
        listRegiones[i].dMedia = listRegiones[i].dMedia/listRegiones[i].totalFijos;
    }

}

void MainWindow::addListRegiones(Point actual){
    region aux;
    aux.pInicio = actual;
    aux.nPuntos = points;
    aux.gMedio  = grisMedio/points;
    std::vector<Point> list;
    aux.dMedia = 0.0;
    aux.totalFijos = 0;
    listRegiones.push_back(aux);
    std::cout << "added region" << std::endl;
}

void MainWindow::iniciar_disparity(){
    std::cout << "EMPEZAMOS" << std::endl;
    pizda = corners(grayImageI);
    std::cout << "SACADOS PUNTOS IZDA" << std::endl;
    pdcha = corners(grayImageD);
    std::cout << "SACADOS PUNTOS DCHA" << std::endl;
    std::cout << "COMPARAMOS" << std::endl;
    compareCorners(pizda, pdcha);
    std::cout << "SEGMENTACION" << std::endl;
    segmentation();
    std::cout << "DISPARIDAD NO FIJOS" << std::endl;
    disparityNonFixed();
    // print disparidad
    print_disparity();
}

void MainWindow::print_disparity(){
    for (int i = 0; i < disparity.rows; i++) {
        for (int j = 0; j < disparity.cols; j++) {
            float value = disparity.at<float>(i,j)*3*grayImageD.cols/320.;
            if(value < 0.){
                destGrayImage.at<uchar>(i,j) = 0.;
            } else {
                if(value > 255.){
                    destGrayImage.at<uchar>(i,j) = 255.;
                } else {
                    destGrayImage.at<uchar>(i,j) = value;
                }
            }
        }
    }
}

void MainWindow::drawCorners(){
    Point aux;
    for(int i = 0; i < pizda.size(); i++)
    {
        aux.x = pizda[i][1];
        aux.y = pizda[i][0];
        if(std::find(puntosI.begin(), puntosI.end(), aux) == puntosI.end()){
            visorSI->drawSquare(QPoint(pizda[i][1],pizda[i][0]),2,2, Qt::red, true);
        } else {
            visorSI->drawSquare(QPoint(pizda[i][1],pizda[i][0]),2,2, Qt::green, true);
        }
    }
    for(int i = 0; i < pdcha.size(); i++)
    {
        aux.x = pdcha[i][1];
        aux.y = pdcha[i][0];
        if(std::find(puntosD.begin(), puntosD.end(), aux) == puntosD.end()){
            visorSD->drawSquare(QPoint(pdcha[i][1],pdcha[i][0]),2,2, Qt::red, true);
        } else{
            visorSD->drawSquare(QPoint(pdcha[i][1],pdcha[i][0]),2,2, Qt::green, true);
        }
    }
}

void MainWindow::disparityNonFixed(){
    for (int i = 0; i < disparity.rows; i++) {
        for(int j = 0; j < disparity.cols; j++){
            if(fixedPoints.at<uchar>(i,j) != 1){
                int id = imgRegiones.at<int>(i,j);
                float disp = listRegiones[id].dMedia;
                disparity.at<float>(i,j) = disp;
            }
        }

    }
}

void MainWindow::load_disparityImg(){
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load an Image",
                                                    "/home/",
                                                    "Image Files (*.png *.jpg)");
    dispImg = imread(filename.toStdString(), IMREAD_COLOR);
    cv::resize(dispImg, dispImg, Size(320, 240));
    cvtColor(dispImg, trueDispImage, COLOR_BGR2RGB);
}

void MainWindow::update_disparity(QPointF p, int w, int h){
    ui->estimatedDisparity->display(destGrayImage.at<uchar>(p.y(), p.x()));
    ui->trueDisparity->display(trueDispImage.at<uchar>(p.y(), p.x()));
}

void MainWindow::propagateDisparity(){
    for (int i = 3; i < disparity.rows-3; i++) {
        for(int j = 3; j < disparity.cols-3; j++){
            if(fixedPoints.at<uchar>(i,j) == 0){
                int points = 0.;
                float values = 0.;
                for (int k = i-3; k < i+4; k++) {
                    for (int l = j-3; l < j+4; l++) {
                        if(imgRegiones.at<int>(i,j) == imgRegiones.at<int>(k,l)){
                            values += disparity.at<float>(k,l);
                            points++;
                        }
                    }
                }
                if(points > 0){
                    float result = values/(float)points;
                    disparity.at<float>(i,j) = result;
                }
            }
        }
    }
   std::cout << "propagada" << std::endl;
   print_disparity();
}

void MainWindow::update_3DPosition(QPointF p, int w, int h){
    ui->xEstimated->display(p.x()*1239/320);
    ui->yEstimated->display(p.y()*1110/240);
    //ui->zEstimated->display();
}
