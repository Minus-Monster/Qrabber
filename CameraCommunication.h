#ifndef CAMERACOMMUNICATION_H
#define CAMERACOMMUNICATION_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QComboBox>

namespace Ui {
class CameraCommunication;
}

class CameraCommunication : public QDialog
{
    Q_OBJECT

public:
    explicit CameraCommunication(QWidget *parent = nullptr);
    ~CameraCommunication();

    bool connectSerial();
    void disconnectSerial();
    bool loadLibrary(std::string &libPath);

    void print(QString message, int num=0);

    int getValue(const char *addr);
    bool setValue(const char *addr, int value);

    void refreshAlldata();
    void clearAll();

    double getExposureTime();
    QString getExposureMode();
    int getFrameCount();

public slots:
    void setExposureMode(QString mode);
    void setFrameCount(int frame);
    void setExposureTime(double time);

signals:
    void refreshed();


private:
    Ui::CameraCommunication *ui;
    void *serialRefPtr;

    bool initInterface();
};

#endif // CAMERACOMMUNICATION_H
