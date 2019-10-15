#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include <QList>
#define pr(x) ui->text->append(x)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QList<QBluetoothHostInfo> localAdapters = QBluetoothLocalDevice::allDevices();

    ui->setupUi(this);

    ui->BTAddr->setText("24:0A:C4:06:D6:8E");

    if (localAdapters.count() > 1) {
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent(localAdapters.at(1).address());
        ui->text->append(localAdapters.at(1).address().toString());
    } else {
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    }
    discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(discoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));


    BTSock = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(BTSock, &QBluetoothSocket::connected, this, &MainWindow::BTSock_connected);
    connect(BTSock, static_cast<void(QBluetoothSocket::*)(QBluetoothSocket::SocketError)>(&QBluetoothSocket::error),
        [=](QBluetoothSocket::SocketError){ pr(BTSock->errorString()); });
    connect(BTSock, &QBluetoothSocket::readyRead, this, &MainWindow::readSocket);

/*
    modbusDevice = new QModbusRtuSerialMaster(this);
    //modbusDevice = new QModbusDevicePrivate(this);
    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) { pr("MODBUS: " + modbusDevice->errorString()); });
*/

    timer_1s = new QTimer(this);
    connect(timer_1s, &QTimer::timeout, this, &MainWindow::timer1Sact);
    timer_1s->start(500);

    timer_Qs = new QTimer(this);
    connect(timer_Qs, &QTimer::timeout, this, &MainWindow::timerQSact);
    timer_Qs->start(50);

    QMainWindow::showFullScreen();
}

MainWindow::~MainWindow()
{
    if (BTSock->ConnectedState == QBluetoothSocket::ConnectedState) BTSock->disconnect();
    delete ui;
    delete BTSock;
}

void MainWindow::addDevice(const QBluetoothDeviceInfo &info)
{
    QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());
    QListWidgetItem *item = new QListWidgetItem(label);

    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) item->setTextColor(QColor(Qt::green));

    ui->text->append(label);
    ui->list->addItem(item);
}

void MainWindow::on_ConnectButton_clicked()
{
    //QString address = ui->list->currentItem()->text().section(" ", 0, 0);
    QString address = ui->BTAddr->text();

    if (BTSock->state() == QBluetoothSocket::ConnectedState) {
        BTSock->disconnectFromService();
        ui->ConnectButton->setText("Connect");
    } else {
        ui->text->append("Connecting :" + address);
        BTSock->connectToService(QBluetoothAddress(address), QBluetoothUuid(QBluetoothUuid::SerialPort));
        //BTSock->connectToService(QBluetoothAddress(address), QBluetoothUuid(QBluetoothUuid::L2cap));
    }
}

void MainWindow::BTSock_connected() {
    pr("BT Socket connected");
    pr(BTSock->peerName());
    ui->ConnectButton->setText("Disconnect");
    BTSock->write("*getS:xxx#");
    /*
    if (!modbusDevice)
        return;

    if (modbusDevice->state() != QModbusDevice::ConnectedState) {
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, "/dev/ttyUSB1");
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QSerialPort::Baud38400);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
        modbusDevice->setTimeout(100);
    } else {
        modbusDevice->disconnectDevice();
    }
    */
}


void MainWindow::readSocket(){
    qint64 i;
    if (!BTSock) return;

    while(BTSock->bytesAvailable()) {
        i = BTSock->readLine(bt_buf + bt_bufi, 128);
        bt_bufi += i;
        //QThread::msleep(1000);
    }
    //pr(QString::number(bt_bufi));
    //pr(QChar(bt_buf[bt_bufi - 1]));
    if (bt_bufi > 128) bt_bufi = 0;
    //bt_buf[bt_bufi] = 0;
}

void MainWindow::scanFinished() {
    ui->scan->setEnabled(true);
//    ui->inquiryType->setEnabled(true);

}

void MainWindow::on_scan_clicked()
{
    ui->list->clear();
    discoveryAgent->start();
    //discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    ui->scan->setEnabled(false);
//    ui->inquiryType->setEnabled(false);

}

void MainWindow::on_list_itemDoubleClicked(QListWidgetItem *item) {
    ui->BTAddr->setText(item->text().section(" ", 0, 0));
}

void MainWindow::timer1Sact() {
    if (BTSock->state() == QBluetoothSocket::ConnectedState) {
        BTSock->write("*getV:xxx#");
    }
}

void MainWindow::timerQSact() {
    //QString s;
    //pr(s.sprintf("ind=%d str=%s last=%c", bt_bufi, bt_buf, bt_buf[bt_bufi - 1]));
    if (bt_bufi && strchr(bt_buf, '*') != nullptr && strchr(bt_buf, '#') != nullptr) {
        //pr("GOT IT!");
        *(strchr(bt_buf, '#') + 1) = 0;
        CmdReceiver(QString::fromLocal8Bit(strchr(bt_buf, '*')));
        bt_bufi = 0;
        bt_buf[0] = 0;
    }
}

void MainWindow::CmdReceiver(QString r) {
    //pr(r);
    QStringList tmpList = r.split(QRegExp("(\\,|\\:|\\#)"));
    //pr(tmpList.join("&"));
    if (tmpList.at(0) == "*Sets") { // MaxTemp, MinTemp, Period
        ui->MaxTempSB->setValue(static_cast<int>(tmpList.at(1).toFloat()));
        ui->MinTempSB->setValue(static_cast<int>(tmpList.at(2).toFloat()));
        ui->PeriodSB->setValue(static_cast<int>(tmpList.at(3).toFloat()));
    }
    if (tmpList.at(0) == "*Vals") { // Temp, Timer, Heat, Flow
        uint i = tmpList.at(2).toUInt() / 1000;
        QString a;
        a.sprintf("%2d:%02d",i/60,i%60);
        ui->lcd1->display(QString::number(tmpList.at(1).toDouble(), 'f', 1));
        ui->lcd2->display(a);
        if (tmpList.at(3).toInt()) ui->HeatLabel->setStyleSheet("QLabel { color : red; }");
        else ui->HeatLabel->setStyleSheet("QLabel { color : blue; }");
        if (tmpList.at(4).toInt()) ui->FlowLabel->setStyleSheet("QLabel { color : green; }");
        else ui->FlowLabel->setStyleSheet("QLabel { color : blue; }");
        switch(tmpList.at(5).toInt()) {
        case STATE_IDLE : ui->StateLabel->setText("Idle"); break;
        case STATE_WORK : ui->StateLabel->setText("Work"); break;
        case STATE_STOP : ui->StateLabel->setText("Stop"); break;
        case STATE_END : ui->StateLabel->setText("End"); break;
        }
    }
}

void MainWindow::on_GetSets_clicked()
{
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    BTSock->write("*getS:xxx#");
}

void MainWindow::on_GetStatus_clicked()
{
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    BTSock->write("*getV:xxx#");
}

void MainWindow::on_TestButton_clicked()
{
    QString a = "15.2";
    pr(QString::number(a.toDouble()));
    ui->lcd2->display("18:02");
    ui->HeatLabel->setStyleSheet("QLabel { color : green; }");
}

void MainWindow::on_LetStart_clicked() {
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    BTSock->write("*strt:xxx#");
}

void MainWindow::on_LetStop_clicked() {
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    BTSock->write("*stop:xxx#");
}

void MainWindow::on_SetMaxTemp_clicked(){
    char s[80];
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    sprintf(s, "*smaT:%d#", ui->MaxTempSB->value());
    BTSock->write(s);
}

void MainWindow::on_SetMinTemp_clicked() {
    char s[80];
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    sprintf(s, "*smiT:%d#", ui->MinTempSB->value());
    BTSock->write(s);
}

void MainWindow::on_SetPeriod_clicked(){
    char s[80];
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    sprintf(s, "*setP:%d#", ui->PeriodSB->value());
    BTSock->write(s);
}

void MainWindow::on_Reset_clicked() {
    if (BTSock->state() != QBluetoothSocket::ConnectedState) return;
    BTSock->write("*rest:xxx#");
}

void MainWindow::on_QuitB_clicked() {
    QCoreApplication::quit();
}
