#ifndef SERIALPOAT_H
#define SERIALPOAT_H

#include <QWidget>
#include <QSerialPort>

namespace Ui {
class serialPoat;
}

class serialPoat : public QWidget
{
    Q_OBJECT

public:
    explicit serialPoat(QWidget *parent = nullptr);
    ~serialPoat();

private slots:

    void on_comboBoxNo_currentIndexChanged(int index);

    void on_pushButtonOpen_clicked();

    void on_pushButtonSend_clicked();

    void MyComRevSlot();

    void on_sendAbcOrHex_clicked();

    void on_revAbcOrHex_clicked();

private:
    Ui::serialPoat *ui;

    QSerialPort MyCom;//串口对象，项目中唯一的串口对象


};

#endif // SERIALPOAT_H
