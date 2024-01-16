#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);

    m_timer = new QTimer(this);  // �����ʱ����������ض�ʱ��ɼ�����
    m_timer->setInterval(100);   // ��� 100 ms �ɼ�һ������

    connect(m_timer, &QTimer::timeout, this, &Widget::on_timerTimeout);

    // ����һ�� ModbusRTU ͨ�ŵ���������
    modbusDevice = new QModbusRtuSerialMaster;
    // �����վ�Ѿ��������豸���ӣ�ռ�ã�����ȡ���������ӳ���
    if (modbusDevice->state() == QModbusDevice::ConnectedState) {
        modbusDevice->disconnect();
    }

    // ע�� com ��Ҫ�ڵ����豸����������飬������ȷ com �ţ���Ȼ������ʧ��
    // ֻҪ com ��������ȷ������������ modbus�豸������������ȷ��ͨ��
    modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant("com5"));

    // Ҫ����ȷͨ�ţ��������� modbusDevice �Ĳ����ʡ�У��λ��ֹͣλ����żУ���ѡ��
    // ���ĸ��������ô���Ҳ�����ӳɹ�������ȡ����ʱ�ᱨ��"Response timeout."
    modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QSerialPort::Baud9600);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);

    modbusDevice->setTimeout(1000);  // 1s ��ȡһ������
    modbusDevice->setNumberOfRetries(3);

    // ���� modbus �豸
    bool ok = modbusDevice->connectDevice();
    if (ok) {
        qDebug() << "Connect success!";
    } else {
        qDebug() << "Connect falied!";
    }
}

Widget::~Widget() {
    if (modbusDevice) {
        modbusDevice->disconnectDevice();
    }

    delete modbusDevice;
    delete ui;
}

void Widget::readyRead() {
    // QModbusReply �����洢���Կͻ��˵����ݣ�sender() ���ط����źŵĶ����ָ��
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (reply == nullptr) {
        qDebug() << "No response!";
        return;
    }

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit rev_data = reply->result();
        qDebug() << rev_data.values();  // ����ֵ�� QVector����ʵ�� quint16 ����

        // �� vector ��ͷ�ļ��ж���Ϊȫ�ֱ�����ÿ��ֻ��ȡһ���Ĵ������˴� value(0) ׷�ӵ� vector ��
        vector.append(rev_data.value(0));
        qDebug() << "vector: " << vector;

        quint16 tempartureData = rev_data.value(0);  // index Ϊ 0 ������ŵ����¶�ԭʼ���ݣ�16 λ���ͣ�
        quint16 humidityData   = rev_data.value(1);  // index Ϊ 1 ������ŵ���ʪ��ԭʼ���ݣ�16 λ���ͣ�

        // ��ʵ��ʪ��ֵ��Ҫ���� 10
        qreal temparture = tempartureData / 10.0;
        qreal humidity   = humidityData / 10.0;

        QString str1 = QString("%1").arg(temparture);  // ���԰�������������ת��Ϊ�ַ���
        //QString str2 = QString("%1").arg(humidity);
        QString str2 = QString::number(humidity, 'g', 4);  // �Ѹ���������ת��Ϊ�ַ���

        ui->temEdit->setText(str1);
        ui->humEdit->setText(str2);

        vector.clear();  // ÿ�βۺ���������ɺ�Ҫ�� vector ���������������һֱ׷��
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        // ����ȡ�ļĴ�����ʼλ�ô���ʱ���˴����� ��Modbus Exception Response.��
        qDebug() << "Read modbus error!" << reply->errorString();
    } else {
        // ���豸��ַ��������ݷ���ʱ�����߼Ĵ�������ѡ��ʱ������ ��response timeout��
        qDebug() << "Other error!" << reply->errorString();
    }

    reply->deleteLater();
}

// ��ȡһ������
void Widget::on_btnRead_clicked() {
    // �ӵ�ַ 0x0101 ��ʼ��ȡ 2 �����ּĴ�����ֵ���������Ǵ������ĵ�ַ
    // OModbusDataUnit data(QModbusDataUnit::HoldingRegisters, 0x0101, 2);

    // �ӵ�ַ 0x0102 ��ʼ��ȡ 2 �����ּĴ�����ֵ���������Ǵ������ĵ�ַ
    // OModbusDataUnit data(QModbusDataUnit::HoldingRegisters, 0x0102, 2);

    // M2003 ����Ĵ�����ַ InputRegisters 0x0000 ��Ӧ IN0 ����˿ڣ�M2003 ���ݲɼ�ģ���ַΪ 0x01
    // M2111 ����Ĵ�����ַ InputRegisters 0x0064 ��Ӧ RTD0 ����˿ڣ�M2111 �ȵ�ż���ݲɼ�ģ���ַΪ 0x02
    // �ӼĴ�����ַ Ox01 ��ʼ��ȡ 2 ������Ĵ���ֵ���ֱ����¶ȡ�ʪ��

    /*** ���һ�����ڣ��� modbus Э�飬��ȡ��� RS485 �豸(�豸��ַ��ͬ)�еĲ�ͬ�Ĵ������� ***/
    // ׼�� Qt �� modbus ָ�����ݵ�Ԫ QModbusDataunit
    // 0x0001 ��Ҫ��ȡ���ݵ�����Ĵ�����ʼ��ַ������ modbus �豸��ַ����2 ��ʾ��ȡ 2 ���Ĵ�������
    QModbusDataUnit data(QModbusDataUnit::InputRegisters, 0x0001, 2);

    // ���� 0x0001Ϊ modubus �豸��ַ�����蹦���룬Qt �Ѿ��Զ�����
    QModbusReply *reply = modbusDevice->sendReadRequest(data, 0x0001);
    if (reply == nullptr) {
        // ��sendReadRequest() �� modbus �豸��ַ���ô��󣬱��� ��qt.modbus: (Client) Device is not connected��
        qDebug() << "Read data failed!" << modbusDevice->errorString();
    }
    if (!reply->isFinished()) {
        connect(reply, &QModbusReply::finished, this, &Widget::readyRead);
    }
}

// ���һ��ʱ���Զ��ɼ�����
void Widget::on_btnAuto_clicked() {
    m_timer->start();
}

// ��ʱ����ʱ
void Widget::on_timerTimeout() {
    QModbusDataUnit data(QModbusDataUnit::InputRegisters, 0x0001, 2);
    QModbusReply *reply = modbusDevice->sendReadRequest(data, 0x0001);

    if (reply == nullptr) {
        qDebug() << "Read data failed!" << modbusDevice->errorString();
    }
    if (!reply->isFinished()) {
        connect(reply, &QModbusReply::finished, this, &Widget::readyRead);
    }
}

// ��������
void Widget::on_btnStop_clicked() {
    m_timer->stop();
}
