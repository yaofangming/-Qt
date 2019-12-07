#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    //初始化设置端口
    ui->sentdata->setEnabled(true);
    ui->sentdata->setEnabled(false);
}
//串口点击事件
void Widget::on_select_port_clicked()
{
     select_port();
}
//扫描串口
void Widget:: select_port()
{
        allPort.clear();
        foreach(const QSerialPortInfo &info, QSerialPortInfo ::availablePorts())
        {
            QSerialPort  serial;
            serial.setPort(info);
            if(serial.open(QIODevice::ReadWrite))
            {
                if(-1 == allPort.indexOf(serial.portName()))
                    allPort.append(serial.portName());
                serial.close();
            }
        }
        ui->portName->clear();
        ui->portName->addItems(allPort);


}

//CRC_16校验
int  Widget:: usMBCRC16( int  pu[], int usLen )
{

    int ucCRCHi = 0xFF;
    int ucCRCLo = 0xFF;
    int i=0;
    int iIndex;
    while( usLen-- )
     {
        iIndex = ucCRCLo ^ pu[i++] ;
        ucCRCLo = (int)( ucCRCHi ^ aucCRCHi[iIndex] );
        ucCRCHi = aucCRCLo[iIndex];
     }
        return ( ucCRCHi << 8 | ucCRCLo );
}

Widget::~Widget()
{
    delete ui;
}

//16进制接收数据
void Widget::readData()
{
    QByteArray buf;
    buf = serial->readAll();
    if(!buf.isEmpty())
    {
        if(ui->checkShowReciveHex->isChecked())
        {
            QString str = ui->textEditRecive->toPlainText();
            QString c,d,yan1,yan2,yan3,lin1,lin2,lin3;
            int yan0,lin0;

            // byteArray 转 16进制
            QByteArray temp = buf.toHex();
            str+=tr(temp);

            //对str进行切分转换成10进制数保存到数据库中
            //对f8030412341234fedc进行选取数值
            //此部分也可由.mid()来实现更方便
            str += "  ";
            d=tr(temp);
            c=d.remove(0,6);
            c=c.remove(9,4);

            yan1=c.left(4);
            yan2=yan1.right(2);
            yan3=yan1.left(2);
            yan0=yan3.toInt(0,16)*256+yan2.toInt(0,16);//获取烟浓度

            lin1=c.right(4);
            lin2=lin1.right(2);
            lin3=lin1.left(2);
            lin0=lin3.toInt(0,16)*256+lin2.toInt(0,16);//获取磷化氢浓度

            if((yan0-oldyan >50)|(oldyan-yan0>50)|(lin0-oldlin>20)|(oldlin-lin0>20))
            {
              opeartDB(yan0,lin0);
              oldyan=yan0;
              oldlin=lin0;
            }
            ui->textEditRecive->clear();
            ui->textEditRecive->append(str);
            buf.clear();    //清空缓存区
        }
    }
}

//打开端口事件
void Widget::on_openPort_clicked()
{

    QString sDbNm = "C:/Users/22060/Desktop/Database2.mdb";
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");//设置数据库驱动
    QString dsn = QString("DRIVER={Microsoft Access Driver (*.mdb)}; FIL={MS Access};DBQ=%1;").arg(sDbNm);//连接字符串
    db.setDatabaseName(dsn);//设置连接字符串
    db.setUserName("");//设置登陆数据库的用户名
    db.setPassword("");//设置密码

    if(ui->portName->count() == 0)
    {
        QMessageBox::about(this,"打开串口失败","没有可用串口,扫描串口后重试！"); return;
    }
    if(ui->openPort->text()==tr("打开串口"))
    {
        serial = new QSerialPort;
        //设置串口名
        serial->setPortName(ui->portName->currentText());
        //打开串口
        serial->open(QIODevice::ReadWrite);
        //设置波特率
        serial->setBaudRate(ui->portBaudRate->currentText().toInt());
        //设置数据位数
        switch(ui->portDataBits->currentIndex())
        {
        case 8: serial->setDataBits(QSerialPort::Data8); break;
        default: break;
        }
        //设置奇偶校验
        switch(ui->portParity->currentIndex())
        {
        case 0: serial->setParity(QSerialPort::NoParity); break;
        default: break;
        }
        //设置停止位
        switch(ui->portStopBits->currentIndex())
        {
        case 1: serial->setStopBits(QSerialPort::OneStop); break;
        case 2: serial->setStopBits(QSerialPort::TwoStop); break;
        default: break;
        }
        //设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);
        //关闭设置菜单使能
        ui->portName->setEnabled(false);
        ui->portBaudRate->setEnabled(false);
        ui->portDataBits->setEnabled(false);
        ui->portParity->setEnabled(false);
        ui->portStopBits->setEnabled(false);
        ui->select_port->setEnabled(false);
        ui->sentdata->setEnabled(true);
        ui->openPort->setText(tr("关闭串口"));

        bool ok = db.open();
        if (!ok)
        {
            QMessageBox messageBox;
            messageBox.setText("Database error");
            messageBox.exec();
            db.close();
        }
        //连接信号槽
        QObject::connect(serial, &QSerialPort::readyRead, this, &Widget::readData);
    }
    else
    {
        //关闭串口
        serial->clear();
        serial->close();
        serial->deleteLater();
        //恢复设置使能
        ui->portName->setEnabled(true);
        ui->portBaudRate->setEnabled(true);
        ui->portDataBits->setEnabled(true);
        ui->portParity->setEnabled(true);
        ui->portStopBits->setEnabled(true);
        ui->select_port->setEnabled(true);
        ui->sentdata->setEnabled(false);
        ui->openPort->setText(tr("打开串口"));
        db.close();

    }    
}

//16进制发送数据
void Widget::on_sentdata_clicked()
{
    if(ui->checkShowSend->isChecked())
    {
        //接收两个阈值的数据进行处理

        //烟雾阈值设置
        int  a=ui->lineEdit->text().toInt()/256;
        int  b=ui->lineEdit->text().toInt()%256;
        QString hex1 = QString("%1").arg(a, 2, 16,QChar('0'));
        QString hex2 = QString("%1").arg(b, 2, 16,QChar('0'));

        //磷化氢阈值设置
        int  c = ui->lineEdit_2->text().toInt()/256;
        int  d = ui->lineEdit_2->text().toInt()%256;
        QString hex3 = QString("%1").arg(c, 2, 16,QChar('0'));
        QString hex4 = QString("%1").arg(d, 2, 16,QChar('0'));
        //设置校验位
        //添加内容
        QString hex5 = QString("%1").arg(5, 2, 16,QChar('0'));
        int ww1=hex1.toInt(0,16);
        int ww2=hex2.toInt(0,16);
        int ww3=hex3.toInt(0,16);
        int ww4=hex4.toInt(0,16);
        int ww5=hex5.toInt(0,16);
        int num[5] = {ww5,ww1,ww2,ww3,ww4};
        int num1  = usMBCRC16(num,5);
        QString ff= QString("%1").arg(num1,4,16,QChar('0'));        
        QString ee=ff.right(2);
        QString dd=ff.left(2);
        QString hh=" ";
        ui->result->setText(hex5+hh+hex1+hh+hex2+hh+hex3+hh+hex4+hh+ee+hh+dd);
        //发送数据
        QString str=ui->result->text();
        QString A[20]={0};
        int counter=0;
        for(int i=0;i<str.length();i+=2)
        {
            if(str[i] == ' ')
            {
                --i;
                continue;
            }
            A[counter] = str.mid(i,2);
            SendBuff.append(A[counter++].toInt(0,16));
        }        
        serial->write(SendBuff);
        SendBuff.clear();
    }
}

//清除textEditRecive中的内容
void Widget::on_buttonClearRecive_clicked()
{
    ui->textEditRecive->clear();
}



//对数据的存储进行相关操作
void Widget::opeartDB(int yan,int lin)
{
    //获取时间
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy/MM/dd hh:mm:ss");

    QString time=current_date.left(10);//"yyyy/MM/dd"
    QString time1=current_date.right(8);//"hh:mm:ss"

    QSqlQuery query;
    QString select_all_sql = "select * from ["+ time +" ]";
    QString create_sql = "create table ["+ time +" ](Times  DATETIME, Nums  INTEGER  ,Data1  INTEGER)";
    QString insert_sql = "insert into ["+ time +" ] VALUES (:Times, :Nums, :Data1)";

    if(query.exec(select_all_sql))
    {
        query.prepare(insert_sql);
        query.bindValue(":Times", time1);
        query.bindValue(":Nums", yan);
        query.bindValue(":Data1", lin);
        query.exec();
    }
    else
    {
        if(query.exec(create_sql))
        {
            query.prepare(insert_sql);
            query.bindValue(":Times", time1);
            query.bindValue(":Nums", yan);
            query.bindValue(":Data1", lin);
            query.exec();
        }
          else
        {
            qDebug()<<"创建不成功1";
        }
    }
}


























