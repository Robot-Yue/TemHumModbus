#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QModbusRtuSerialMaster> // QModbus ͨ�ŵĿͻ�����
#include <QModbusDataUnit>        // QModbus ���ݵ�Ԫ
#include <QModbusClient>          // �ͻ���
#include <QModbusReply>           // �ͻ��˷��ʷ�������õ��Ļظ����洢���Կͻ��˵�����
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
    void readyRead();           // ���ݶ�ȡ�Ĳۺ���
    void on_btnRead_clicked();  // ��ȡһ������
    void on_btnAuto_clicked();  // ���һ��ʱ���Զ��ɼ�����
    void on_timerTimeout();     // ��ʱ����ʱ
    void on_btnStop_clicked();  // ��������

private:
    QModbusClient *modbusDevice = nullptr;

    Ui::Widget *ui;

    QVector<quint16> vector;  // ����ժ¼ reply ���յ������ݣ�reply->result() ���ص��� QVector��
    quint16 value1;
    quint16 value2;
    quint16 voltage;
    QTimer *m_timer;
};
#endif // WIDGET_H
