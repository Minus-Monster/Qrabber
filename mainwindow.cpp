#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAction>
#include <QDockWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), comm(new CameraCommunication(this)), cal(new CalibrationDirectoryDialog(this))
{
    ui->setupUi(this);

    QDockWidget *consoleWidget = new QDockWidget;
    consoleWidget->setWidget(&console);
    consoleWidget->setWindowTitle("Debug Console");
    addDockWidget(Qt::BottomDockWidgetArea, consoleWidget);

    actionStart = new QAction(QIcon(":/Resources/Icon/icons8-play-48.png"), "Start", this);
    actionStart->setEnabled(false);
    connect(actionStart, &QAction::triggered, this, [this]{
        if(grabber->isInitialized()){
            if(spinboxGrabCnt->value()==0){
                grabber->grabThreadLoop();
            }else{
                // Check what the detector's exposure mode is
                // Fg_setParameter(fg, FG_TREGGERMODE, ASYNC_SOFTWARE_TRIGGER, 0);
                // Fg_setParameter(fg, FG_EXPOSURE, &nExposureInMicroSec, nCamPort);
                // Fg_setParameter(fg, FG_EXSYNCON, &nExsyncOn, nCamPort)
                // Fg_AcquireEx(fg, nCamPort, GRAB_INFINITE, ACQ_STANDARD, pMem0))
                // while (Fg_sendSoftwareTrigger(fg, nCamPort) == FG_SOFTWARE_TRIGGER_BUSY)
                //    ;
            }
        }
    });
    actionStop = new QAction(QIcon(":/Resources/Icon/icons8-stop-48.png"), "Stop", this);;
    actionStop->setEnabled(false);
    connect(actionStop, &QAction::triggered, this, [this]{
        grabber->stopThreadLoop();
    });
    actionConf = new QAction(QIcon(":/Resources/Icon/icons8-note-48.png"), "Configuration", this);
    actionSerial = new QAction(QIcon(":/Resources/Icon/icons8-rs-232-male-48.png"), "Serial", this);;

    QAction *actionSeperator = new QAction(this);
    actionSeperator->setSeparator(true);

    QAction *actionCalFolder = new QAction(QIcon(":/Resources/Icon/icons8-folder-48.png"), "Set Calibration folder", this);
    connect(actionCalFolder, &QAction::triggered, this, [this]{
        cal->show();
    });

    spinboxGrabCnt = new QSpinBox(ui->widget);
    spinboxGrabCnt->setEnabled(false);
    spinboxGrabCnt->setRange(0, 1000);

    ui->widget->addAction(actionCalFolder);
    ui->widget->addAction(actionSerial);
    ui->widget->addAction(actionConf);
    ui->widget->addAction(actionSeperator);
    ui->widget->addAction(actionStop);
    ui->widget->addWidget(spinboxGrabCnt);
    ui->widget->addAction(actionStart);
    ui->widget->setFPSEnable(true);
    connect(actionConf, &QAction::triggered, this, [this]{
        grabber->getWidget()->setParent(this, Qt::WindowFlags::enum_type::Dialog);
        grabber->getWidget()->show();
    });
    connect(actionSerial, &QAction::triggered, this, [this]{
        comm->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setGrabber(Qylon::Grabber *newGrabber){
    grabber= std::move(newGrabber);

    connect(grabber, &Qylon::Grabber::sendImage, this, [this](const QImage &image, unsigned int num){
        ui->widget->setImage(image);
    }, Qt::QueuedConnection);
    connect(grabber, &Qylon::Grabber::loadedApplet, this, [this]{
        actionStart->setEnabled(true);
        spinboxGrabCnt->setEnabled(true);
    });
    connect(grabber, &Qylon::Grabber::grabbingState, this, [this](bool isGrabbing){
        actionStop->setEnabled(isGrabbing);
        actionStart->setEnabled(!isGrabbing);
        spinboxGrabCnt->setEnabled(!isGrabbing);
    });

    cal->setGrabber(grabber);
}

void MainWindow::append(const QString &msg)
{
    QString text = msg;
    if(!msg.contains("[QYLON]")){
        text = "[SYSTEM] " + QTime::currentTime().toString() +" : " + msg;
    }
    console.append(text);
}
