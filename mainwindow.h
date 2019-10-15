#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QBluetoothSocket>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothdevicediscoveryagent.h>
/*
#include <QtBluetooth/QLowEnergyService>
#include <QLowEnergyController>
#include <QLowEnergyServiceData>
#include <QLowEnergyCharacteristic>
*/
/*
#include <QSerialPort>
#include <QModbusDataUnit>
#include <QModbusRtuSerialMaster>
*/
#include <QListWidget>
#include <QThread>
#include <QTimer>

#define STATE_IDLE      0
#define STATE_WORK      1
#define STATE_STOP      2
#define STATE_END       3

//#include "leconnection.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void on_ConnectButton_clicked();
    void BTSock_connected();
    void readSocket();
    void scanFinished();
    void on_scan_clicked();
    void on_list_itemDoubleClicked(QListWidgetItem *item);
    void timer1Sact();
    void timerQSact();
    void CmdReceiver(QString r);
    void on_GetSets_clicked();
    void on_GetStatus_clicked();
    void on_TestButton_clicked();
    void on_LetStart_clicked();
    void on_LetStop_clicked();
    void on_SetMaxTemp_clicked();
    void on_SetMinTemp_clicked();
    void on_SetPeriod_clicked();
    void on_Reset_clicked();

    void on_QuitB_clicked();

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothLocalDevice *localDevice;
    QBluetoothSocket *BTSock = nullptr;
    //QModbusClient *modbusDevice;
    char bt_buf[256];
    int bt_bufi = 0;
    QTimer *timer_1s, *timer_Qs;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
