#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardItemModel>

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
    yProcesada.create(240,320,CV_8UC1);

    visorHistoS = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameD);


    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);


    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    /***************************************************************************/
    connect(ui->loadButton,SIGNAL(clicked(bool)),this,SLOT(chooseImage(bool)));
    connect(ui->saveButton,SIGNAL(clicked()),this,SLOT(saveImage()));
    connect(ui->kernelButton, SIGNAL(clicked()), this, SLOT(kernelImg()));
    connect(ui->pixelTButton, SIGNAL(clicked()), this, SLOT(pixelTImg()));
    connect(ui->operOrderButton, SIGNAL(clicked()), this, SLOT(operOrderImg()));
    connect(lFilterDialog.okButton, SIGNAL(clicked(bool)), this, SLOT(disconnectKernel()));
    connect(lFilterDialog.sizeKernel,SIGNAL(valueChanged(int)) ,this,SLOT(kernelImg()));
    timer.start(30);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
}

void MainWindow::compute()
{
    //Captura de imagen

    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;

        cv::resize(colorImage, colorImage, Size(320, 240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }


    //Procesamiento
    Mat *img0, *imgD;
    std::vector<cv::Mat> vectorO(3);
    if(ui->colorButton->isChecked())
    {
        cvtColor(colorImage, imgYUV, COLOR_RGB2YUV);
        split(imgYUV,vectorO);
        img0 = &vectorO[0];
        imgD = &yProcesada;
    } else {
        img0 = &grayImage;
        imgD = &destGrayImage;
    }

    int value = ui->operationComboBox->currentIndex();
    chooseOption(value, img0, imgD);


    //Actualización de los visores
    if(ui->colorButton->isChecked()){
        std::vector<cv::Mat> vectorDest(3);
        Mat finalYUV;
        vectorDest[0] = yProcesada;
        vectorDest[1] = vectorO[1];
        vectorDest[2] = vectorO[2];
        merge(vectorDest, finalYUV);
        cvtColor(finalYUV, destColorImage, COLOR_YUV2RGB);
    }
    updateHistograms(*img0, visorHistoS);
    updateHistograms(*imgD, visorHistoD);

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();
    visorHistoS->update();
    visorHistoD->update();

}

void MainWindow::kernelImg(){
    lFilterDialog.show();
    lFilterDialog.tableWidget->setRowCount(lFilterDialog.sizeKernel->value());
    lFilterDialog.tableWidget->setColumnCount(lFilterDialog.sizeKernel->value());
    int x = lFilterDialog.sizeKernel->value()/2;
    int y = lFilterDialog.sizeKernel->value()/2;

    for(int i = 0; i < lFilterDialog.sizeKernel->value(); i++){
        for(int j = 0; j < lFilterDialog.sizeKernel->value(); j++){
            lFilterDialog.tableWidget->setItem(i, j, new QTableWidgetItem(0));
            if(i==x and j==y)
                 lFilterDialog.tableWidget->setItem(x, y, new QTableWidgetItem("1.0"));
            else
                lFilterDialog.tableWidget->setItem(i, j, new QTableWidgetItem("0.0"));
        }
    }
    lFilterDialog.tableWidget->show();
}

void MainWindow::disconnectKernel(){
    lFilterDialog.close();
}

void MainWindow::pixelTImg(){
    pixelTDialog.show();
    connect(pixelTDialog.okButton, SIGNAL(clicked()), this, SLOT(closePixelTImg()));
}

void MainWindow::closePixelTImg(){
    pixelTDialog.close();
}

void MainWindow::operOrderImg(){
    operOrderDialog.show();
    connect(operOrderDialog.okButton, SIGNAL(clicked()), this, SLOT(closeOperOrderImg()));
}

void MainWindow::closeOperOrderImg(){
    operOrderDialog.close();
}

void MainWindow::updateHistograms(Mat image, ImgViewer * visor)
{
    if(image.type() != CV_8UC1) return;

    Mat histogram;
    int channels[] = {0,0};
    int histoSize = 256;
    float grange[] = {0, 256};
    const float * ranges[] = {grange};
    double minH, maxH;

    calcHist( &image, 1, channels, Mat(), histogram, 1, &histoSize, ranges, true, false );
    minMaxLoc(histogram, &minH, &maxH);

    float maxY = visor->getHeight();

    for(int i = 0; i<256; i++)
    {
        float hVal = histogram.at<float>(i);
        float minY = maxY-hVal*maxY/maxH;

        visor->drawLine(QLineF(i+2, minY, i+2, maxY), Qt::red);
    }

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
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);

    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);

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
        imageWindow.width = pEnd.x()-imageWindow.x;
        imageWindow.height = pEnd.y()-imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::chooseImage(bool clicked){

    start_stop_capture(false);
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
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
    } else{
        start_stop_capture(true);
        ui->captureButton->setChecked(true);
    }
    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::saveImage(){
    Mat imgSave;

    if(ui->colorButton->isChecked()){
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

void MainWindow::chooseOption(int value, Mat *img0, Mat *imgD){
    switch (value) {
        case 0:
            transformPixel(*img0, *imgD);
        break;
        case 1:
            thresholding(*img0, *imgD);
        break;
        case 2:
            equalize(*img0, *imgD);
        break;
        case 3:
            gaussianBlur(*img0, *imgD);
        break;
        case 4:
            medianBlur(*img0, *imgD);
        break;
        case 5:
            linearFilter(*img0, *imgD);
        break;
        case 6:
            dilate(*img0, *imgD);
        break;
        case 7:
            erode(*img0, *imgD);
        break;
        case 8:
            applySeveral(*img0, *imgD);
        break;
        case 9:
            CrossDilation(*img0, *imgD);
        break;
        case 10:
            EllipseErode(*img0, *imgD);
        break;
    }
}


void MainWindow::transformPixel(Mat origen, Mat &dest){
    std::vector<uchar> table(256);
    int i = 0;
    float inc = 0.;

    if(pixelTDialog.applyNeg->isChecked()){
        applyNegative(origen, dest);
    } else {
        int origPixel2 = pixelTDialog.origPixelBox2->value();
        int origPixel3 = pixelTDialog.origPixelBox2->value();
        float point1 = pixelTDialog.newPixelBox1->value();
        float point2 = pixelTDialog.newPixelBox2->value();
        float point3 = pixelTDialog.newPixelBox3->value();
        float point4 = pixelTDialog.newPixelBox4->value();

        for(i=0; i<origPixel2; i++){
            inc = ((point2-point1)/origPixel2)*i;
            table[i] = (uchar) (inc + point1);
        }

        for(i=0; i<origPixel3; i++){
            inc = ((point3-point2)/origPixel3)*i;
            table[i] = (uchar) (inc + point2);
        }

        for(i=origPixel3; i<256; i++){
            inc = ((point4-point3)/255)*i;
            table[i] = (uchar) (inc + point3);
        }

        LUT(origen, table, dest);
    }
}

void MainWindow::thresholding(Mat origen, Mat &dest){
    cv::threshold(origen, dest, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
}

void MainWindow::equalize(Mat origen, Mat &dest){
    cv::equalizeHist(origen, dest);
}

void MainWindow::gaussianBlur(Mat origen, Mat &dest){
    cv::GaussianBlur(origen, dest, Size(3,3), ui->gaussWidthBox->value()/5, ui->gaussWidthBox->value()/5);
}

void MainWindow::medianBlur(Mat origen, Mat &dest){
    cv::medianBlur(origen, dest, 3);
}

void MainWindow::dataKernel(){
    std::cout << "HOLA" << std::endl;
    kernel.create(lFilterDialog.sizeKernel->value(), lFilterDialog.sizeKernel->value(), CV_32F);
    for(int i = 0; i < lFilterDialog.sizeKernel->value(); i++){
        for(int j=0; j < lFilterDialog.sizeKernel->value(); j++){
            kernel.at<float>(i,j) = lFilterDialog.tableWidget->item(i,j)->text().toFloat();
        }
    }
    std::cout << "LEIDO EL KERNEL" << std::endl;
}

void MainWindow::linearFilter(Mat origen, Mat &dest){
    dataKernel();
    cv::filter2D(origen, dest,-1, kernel,Point(-1,-1), lFilterDialog.addedVBox->value());
}

void MainWindow::dilate(Mat origen, Mat &dest){
    Mat imgAux;
    dest.copyTo(imgAux);
    thresholding(origen, imgAux);
    cv::dilate(imgAux, dest, Mat());
}

void MainWindow::erode(Mat origen, Mat &dest){
    Mat imgAux;
    dest.copyTo(imgAux);
    thresholding(origen, imgAux);
    cv::erode(imgAux, dest, Mat());
}

void MainWindow::applySeveral(Mat origen, Mat &dest){
    Mat imgAux;
    origen.copyTo(imgAux);

    if(operOrderDialog.firstOperCheckBox->isChecked()){
        chooseOption(operOrderDialog.operationComboBox1->currentIndex(), &imgAux, &dest);
        dest.copyTo(imgAux);
    }
    if(operOrderDialog.secondOperCheckBox->isChecked()){
        chooseOption(operOrderDialog.operationComboBox2->currentIndex(), &imgAux, &dest);
        dest.copyTo(imgAux);
    }
    if(operOrderDialog.thirdOperCheckBox->isChecked()){
        chooseOption(operOrderDialog.operationComboBox3->currentIndex(), &imgAux, &dest);
        dest.copyTo(imgAux);
    }
    if(operOrderDialog.fourthOperCheckBox->isChecked()){
        chooseOption(operOrderDialog.operationComboBox4->currentIndex(), &imgAux, &dest);
        dest.copyTo(imgAux);
    }

}


// ------------------------------------- AMPLIACIONES ------------------------------------------------
void MainWindow::applyNegative(Mat origen, Mat &dest){
    std::vector<uchar> table(256);
    int s = 255;

    for(int i=0; i<256; i++){
        table[i] = s;
        s--;
    }

    LUT(origen, table, dest);
}

void MainWindow::CrossDilation(Mat origen, Mat &dest){
    Mat imgAux;
    dest.copyTo(imgAux);
    thresholding(origen, imgAux);
    Mat k = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(5,5));
    cv::dilate(imgAux, dest, k);
}

void MainWindow::EllipseErode(Mat origen, Mat &dest){
    Mat imgAux;
    dest.copyTo(imgAux);
    thresholding(origen, imgAux);
    Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5,5));
    cv::erode(imgAux, dest, k);
}
