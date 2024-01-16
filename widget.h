#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QModbusRtuSerialMaster> // QModbus 通信的客户端类
#include <QModbusDataUnit>        // QModbus 数据单元
#include <QModbusClient>          // 客户端
#include <QModbusReply>           // 客户端访问服务器后得到的回复，存储来自客户端的数据
#include <QModbusDevice>
#include <QTimer>
#include <QVector>
#include <QVariant>
#include <QSerialPort>
#include <QDebug>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void readyRead();           // 数据读取的槽函数
    void on_btnRead_clicked();  // 读取一次数据
    void on_btnAuto_clicked();  // 间隔一定时间自动采集数据
    void on_timerTimeout();     // 定时器超时
    void on_btnStop_clicked();  // 结束测量

private:
    QModbusClient *modbusDevice = nullptr;

    Ui::Widget *ui;

    QVector<quint16> vector;  // 用来摘录 reply 中收到的数据（reply->result() 返回的是 QVector）
    quint16 value1;
    quint16 value2;
    quint16 voltage;
    QTimer *m_timer;
};
#endif // WIDGET_H
