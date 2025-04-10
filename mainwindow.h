#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Grabber.h"
#include "CameraCommunication.h"
#include "CalibrationDirectoryDialog.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setGrabber(Qylon::Grabber *newGrabber);

private:
    Ui::MainWindow *ui;
    Qylon::Grabber *grabber;
    CameraCommunication *comm;
    CalibrationDirectoryDialog *cal;
    QAction *actionStart;
    QAction *actionStop;
    QAction *actionConf;
    QAction *actionSerial;
};
#endif // MAINWINDOW_H
