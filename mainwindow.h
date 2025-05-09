#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Grabber/Grabber.h"
#include "CameraCommunication.h"
#include "CalibrationDirectoryDialog.h"
#include "Modules/Console.h"
#include <QSpinBox>
#include <QDockWidget>

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

public slots:
    void append(const QString &msg);

private:
    Ui::MainWindow *ui;
    Qylon::Grabber *grabber;
    CameraCommunication *comm;
    CalibrationDirectoryDialog *cal;
    MQIT::Console console;
    QAction *actionStart;
    QAction *actionStop;
    QAction *actionConf;
    QAction *actionSerial;

    QDockWidget *dockGrabber;
    QDockWidget *dockComm;
};
#endif // MAINWINDOW_H
