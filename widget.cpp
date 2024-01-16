#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);

    m_timer = new QTimer(this);  // 这个定时器用来间隔特定时间采集数据
    m_timer->setInterval(100);   // 间隔 100 ms 采集一次数据

    connect(m_timer, &QTimer::timeout, this, &Widget::on_timerTimeout);

    // 创建一个 ModbusRTU 通信的主机对象
    modbusDevice = new QModbusRtuSerialMaster;
    // 如果从站已经被其他设备连接（占用），就取消本次连接尝试
    if (modbusDevice->state() == QModbusDevice::ConnectedState) {
        modbusDevice->disconnect();
    }

    // 注意 com 号要在电脑设备管理器里面查，填入正确 com 号，不然会连接失败
    // 只要 com 号设置正确，就能连接上 modbus设备，但并不能正确的通信
    modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant("com5"));

    // 要想正确通信，还需设置 modbusDevice 的波特率、校验位、停止位和奇偶校验等选项
    // 这四个参数设置错误，也会连接成功，但读取数据时会报错："Response timeout."
    modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QSerialPort::Baud9600);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
    modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);

    modbusDevice->setTimeout(1000);  // 1s 读取一次数据
    modbusDevice->setNumberOfRetries(3);

    // 连接 modbus 设备
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
    // QModbusReply 这个类存储来自客户端的数据，sender() 返回发送信号的对象的指针
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (reply == nullptr) {
        qDebug() << "No response!";
        return;
    }

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit rev_data = reply->result();
        qDebug() << rev_data.values();  // 返回值是 QVector，其实是 quint16 数组

        // 该 vector 在头文件中定义为全局变量，每次只读取一个寄存器，此处 value(0) 追加到 vector 中
        vector.append(rev_data.value(0));
        qDebug() << "vector: " << vector;

        quint16 tempartureData = rev_data.value(0);  // index 为 0 处，存放的是温度原始数据（16 位整型）
        quint16 humidityData   = rev_data.value(1);  // index 为 1 处，存放的是湿度原始数据（16 位整型）

        // 真实温湿度值均要除以 10
        qreal temparture = tempartureData / 10.0;
        qreal humidity   = humidityData / 10.0;

        QString str1 = QString("%1").arg(temparture);  // 可以把任意类型数据转换为字符串
        //QString str2 = QString("%1").arg(humidity);
        QString str2 = QString::number(humidity, 'g', 4);  // 把浮点型数据转换为字符串

        ui->temEdit->setText(str1);
        ui->humEdit->setText(str2);

        vector.clear();  // 每次槽函数接收完成后要把 vector 清掉，否则该数组会一直追加
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        // 当读取的寄存器起始位置错误时，此处报错 “Modbus Exception Response.”
        qDebug() << "Read modbus error!" << reply->errorString();
    } else {
        // 当设备地址填错无数据返回时、或者寄存器种类选错时，报错 “response timeout”
        qDebug() << "Other error!" << reply->errorString();
    }

    reply->deleteLater();
}

// 读取一次数据
void Widget::on_btnRead_clicked() {
    // 从地址 0x0101 开始读取 2 个保持寄存器的值，这里面是传感器的地址
    // OModbusDataUnit data(QModbusDataUnit::HoldingRegisters, 0x0101, 2);

    // 从地址 0x0102 开始读取 2 个保持寄存器的值，这里面是传感器的地址
    // OModbusDataUnit data(QModbusDataUnit::HoldingRegisters, 0x0102, 2);

    // M2003 输入寄存器地址 InputRegisters 0x0000 对应 IN0 输入端口，M2003 数据采集模块地址为 0x01
    // M2111 输入寄存器地址 InputRegisters 0x0064 对应 RTD0 输入端口，M2111 热电偶数据采集模块地址为 0x02
    // 从寄存器地址 Ox01 开始读取 2 个输入寄存器值，分别是温度、湿度

    /*** 解决一个串口，用 modbus 协议，读取多个 RS485 设备(设备地址不同)中的不同寄存器数据 ***/
    // 准备 Qt 的 modbus 指令数据单元 QModbusDataunit
    // 0x0001 是要读取数据的输入寄存器起始地址（不是 modbus 设备地址），2 表示读取 2 个寄存器数据
    QModbusDataUnit data(QModbusDataUnit::InputRegisters, 0x0001, 2);

    // 参数 0x0001为 modubus 设备地址，无需功能码，Qt 已经自动处理
    QModbusReply *reply = modbusDevice->sendReadRequest(data, 0x0001);
    if (reply == nullptr) {
        // 若sendReadRequest() 中 modbus 设备地址设置错误，报错 “qt.modbus: (Client) Device is not connected”
        qDebug() << "Read data failed!" << modbusDevice->errorString();
    }
    if (!reply->isFinished()) {
        connect(reply, &QModbusReply::finished, this, &Widget::readyRead);
    }
}

// 间隔一定时间自动采集数据
void Widget::on_btnAuto_clicked() {
    m_timer->start();
}

// 定时器超时
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

// 结束测量
void Widget::on_btnStop_clicked() {
    m_timer->stop();
}
