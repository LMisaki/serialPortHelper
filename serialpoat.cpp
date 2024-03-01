#include "serialpoat.h"
#include "ui_serialpoat.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDateTime>

serialPoat::serialPoat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::serialPoat)
{
    ui->setupUi(this);

    QStringList comPort;    //串口列表
    foreach(const QSerialPortInfo info,QSerialPortInfo::availablePorts())
    {
        comPort<<info.portName();   //遍历，获取可用串口
    }
    if(!comPort.isEmpty())  //有可用串口则加入
    {
        ui->comboBoxNo->addItems(comPort);
    }

    connect(&MyCom,SIGNAL(readyRead()),this,SLOT(MyComRevSlot()));
}

serialPoat::~serialPoat()
{
    delete ui;
}

//端口选择
void serialPoat::on_comboBoxNo_currentIndexChanged(int index)
{
    QStringList comPort;    //串口列表
    foreach(const QSerialPortInfo info,QSerialPortInfo::availablePorts())
    {
        comPort<<info.description();   //遍历，获取可用串口
    }
    ui->comPortDescription->setText(comPort[index]);    //显示串口描述
}

//打开串口
void serialPoat::on_pushButtonOpen_clicked()
{
    QSerialPort::BaudRate CombaudRate;  //波特率
    QSerialPort::DataBits ComdataBits;  //数据位
    QSerialPort::StopBits ComstopBits;  //停止位
    QSerialPort::Parity   ComParity;    //校验位

    //获取波特率
    QString Baund = ui->comboBoxComBaud->currentText();
    if(Baund=="1200")
    {
        CombaudRate = QSerialPort::Baud1200;
    }
    else if(Baund=="2400")
    {
        CombaudRate = QSerialPort::Baud2400;
    }
    else if(Baund=="4800")
    {
        CombaudRate = QSerialPort::Baud4800;
    }
    else if(Baund=="9600")
    {
        CombaudRate = QSerialPort::Baud9600;
    }
    else if(Baund=="19200")
    {
        CombaudRate = QSerialPort::Baud19200;
    }
    else if(Baund=="38400")
    {
        CombaudRate = QSerialPort::Baud38400;
    }
    else if(Baund=="57600")
    {
        CombaudRate = QSerialPort::Baud57600;
    }
    else if(Baund=="115200")
    {
        CombaudRate = QSerialPort::Baud115200;
    }
    else
    {
        CombaudRate = QSerialPort::UnknownBaud;
    }

    //获取数据位
    QString data = ui->comboBoxData->currentText();
    if(data=="5")
    {
        ComdataBits = QSerialPort::Data5;
    }
    else if(data=="6")
    {
        ComdataBits = QSerialPort::Data6;
    }
    else if(data=="7")
    {
        ComdataBits = QSerialPort::Data7;
    }
    else if(data=="8")
    {
        ComdataBits = QSerialPort::Data8;
    }
    else
    {
        ComdataBits = QSerialPort::UnknownDataBits;
    }

    //获取停止位
    QString stop = ui->comboBoxStop->currentText();
    if(stop =="1")
    {
        ComstopBits = QSerialPort::OneStop;
    }
    else if(stop =="1.5")
    {
        ComstopBits = QSerialPort::OneAndHalfStop;
    }
    if(stop =="2")
    {
        ComstopBits = QSerialPort::TwoStop;
    }
    else
    {
        ComstopBits = QSerialPort::UnknownStopBits;
    }

    //获取校验方式
    QString check = ui->comboBoxCheck->currentText();
    if(check =="None")
    {
        ComParity = QSerialPort::NoParity;
    }
    else if(check =="Even")
    {
        ComParity = QSerialPort::EvenParity;
    }
    else if(check =="Odd")
    {
        ComParity = QSerialPort::OddParity;
    }
    else if(check =="Space")
    {
        ComParity = QSerialPort::SpaceParity;
    }
    else if(check =="Mark")
    {
        ComParity = QSerialPort::MarkParity;
    }
    else
    {
        ComParity = QSerialPort::UnknownParity;
    }

    //初始化串口：设置端口号，波特率，数据位，停止位，校验方式等
    MyCom.setBaudRate(CombaudRate);
    MyCom.setDataBits(ComdataBits);
    MyCom.setStopBits(ComstopBits);
    MyCom.setParity(ComParity);
    MyCom.setPortName(ui->comboBoxNo->currentText());

    //打开串口 关闭串口
    if(ui->pushButtonOpen->text() == "打开串口")
    {
        if(MyCom.open(QIODevice::ReadWrite) == true)//串口打开成功
        {
            //串口下拉框设置为不可选
            ui->comboBoxCheck->setEnabled(false);
            ui->comboBoxComBaud->setEnabled(false);
            ui->comboBoxData->setEnabled(false);
            ui->comboBoxNo->setEnabled(false);
            ui->comboBoxStop->setEnabled(false);

            //使能相应按钮等
            ui->pushButtonSend->setEnabled(true);
            //ui->checkBoxPeriodicSend->setEnabled(true);
            //ui->checkBoxPeriodicMutiSend->setEnabled(true);
            //ui->lineEditTime->setEnabled(true);
            //ui->checkBoxAddNewShift->setEnabled(true);
            //ui->checkBoxSendHex->setEnabled(true);

            ui->pushButtonOpen->setText("关闭串口");
        }
        else //打开失败，弹出窗口提示！
        {
            QMessageBox::critical(this, "错误提示", "串口打开失败，该端口可能被占用或不存在！");
        }
    }
    else
    {
        MyCom.close();

        ui->pushButtonOpen->setText("打开串口");
        ui->comboBoxCheck->setEnabled(true);
        ui->comboBoxComBaud->setEnabled(true);
        ui->comboBoxData->setEnabled(true);
        ui->comboBoxNo->setEnabled(true);
        ui->comboBoxStop->setEnabled(true);

        //使相应的按钮不可用
        ui->pushButtonSend->setEnabled(false);
//        ui->checkBoxPeriodicSend->setChecked(false);//取消选中
//        ui->checkBoxPeriodicSend->setEnabled(false);
//        ui->checkBoxPeriodicMutiSend->setChecked(false);//取消选中
//        ui->checkBoxPeriodicMutiSend->setEnabled(false);
//        ui->lineEditTime->setEnabled(false);
//        ui->checkBoxAddNewShift->setEnabled(false);
//        ui->checkBoxSendHex->setEnabled(false);
    }

}

//发送按钮
void serialPoat::on_pushButtonSend_clicked()
{
    QByteArray sendData;
    QString str = ui->textSend->text();
    if(ui->sendAbcOrHex->text() == "Abc"){
        if(ui->newShiftCheck->currentText() == "\n"){
            str+="\n";
        }else if(ui->newShiftCheck->currentText() == "\r\n"){
            str+="\r\n";
        }
        sendData = str.toLocal8Bit().data();
    }else{
        sendData = QByteArray::fromHex(str.toLocal8Bit().data());
    }

    MyCom.write(sendData);

    ui->textSend->clear();
}

//串口接收函数
void serialPoat::MyComRevSlot(){
    QByteArray readData =  MyCom.readAll();
    QString str = QString::fromLocal8Bit(readData);

    //是否显示时间
    if(ui->showTimeCheck->checkState()){
        QDateTime time = QDateTime::currentDateTime();
        QString strTime = time.toString("[yyyy-MM-dd hh:mm:ss.zzz] ");
        ui->TextRev->insertPlainText(strTime);
    }

    if(ui->revAbcOrHex->text() == "Hex"){
        str = readData.toHex().toUpper();
        for(int i =2; i<str.length();i+=3){
            str.insert(i,' ');
        }
    }else{
        str = QString::fromLocal8Bit(readData);
    }


    ui->TextRev->appendPlainText(str);
    ui->TextRev->moveCursor(QTextCursor::End);//光标移动到文本末尾
}

//发送 字符串or十六进制
void serialPoat::on_sendAbcOrHex_clicked()
{
    if(ui->sendAbcOrHex->text()=="Abc"){
        ui->sendAbcOrHex->setText("Hex");
    }else{
        ui->sendAbcOrHex->setText("Abc");
    }
}

//接收 字符串or十六进制
void serialPoat::on_revAbcOrHex_clicked()
{
    if(ui->revAbcOrHex->text()=="Abc"){
        ui->revAbcOrHex->setText("Hex");
    }else{
        ui->revAbcOrHex->setText("Abc");
    }
}
