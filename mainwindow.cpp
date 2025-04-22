#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAction>
#include <QDockWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), comm(new CameraCommunication(this)), cal(new CalibrationDirectoryDialog(this))
{
    ui->setupUi(this);
    dockComm = new QDockWidget(this);
    dockGrabber = new QDockWidget(this);
    dockComm->hide();
    dockComm->setWindowTitle("Detector setting");
    dockGrabber->hide();
    dockGrabber->setWindowTitle("Grabber setting");
    addDockWidget(Qt::RightDockWidgetArea, dockGrabber);
    addDockWidget(Qt::RightDockWidgetArea, dockComm);

    QDockWidget *consoleWidget = new QDockWidget;
    consoleWidget->setWidget(&console);
    consoleWidget->setWindowTitle("Debug Console");
    addDockWidget(Qt::BottomDockWidgetArea, consoleWidget);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    actionStart = new QAction(QIcon(":/Resources/Icon/icons8-play-48.png"), "Start", this);
    actionStart->setEnabled(false);
    connect(actionStart, &QAction::triggered, this, [this]{
        if(grabber->isInitialized()){
            if(ui->comboBoxExposureMode->currentText() != "Sequence"){
                grabber->grabThreadLoop();
            }else{
                int frame = ui->spinBoxFrames->value();
                grabber->grabThreadLoop(frame);
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

    ui->widget->addAction(actionCalFolder);
    ui->widget->addAction(actionSerial);
    ui->widget->addAction(actionConf);
    ui->widget->addAction(actionSeperator);
    ui->widget->addAction(actionStop);
    ui->widget->addAction(actionStart);
    ui->widget->setFPSEnable(true);

    actionConf->setCheckable(true);
    connect(actionConf, &QAction::toggled, this, [this](bool toggle){
        if(toggle){
            grabber->getWidget()->setParent(this, Qt::WindowFlags::enum_type::Dialog);
            dockGrabber->setWidget(grabber->getWidget());
            dockGrabber->show();
        }else{
            dockGrabber->hide();
        }
    });
    connect(dockGrabber, &QDockWidget::visibilityChanged, this, [this](bool visible){
        actionConf->blockSignals(true);
        actionConf->setChecked(visible);
        actionConf->blockSignals(false);
        if(visible){
            dockGrabber->show();
        }else{
            dockGrabber->hide();
        }
    });
    actionSerial->setCheckable(true);
    connect(actionSerial, &QAction::triggered, this, [this](bool toggle){
        if(toggle){
            dockComm->setWidget(comm);
            dockComm->show();
        }else{
            dockComm->hide();
        }
    });
    connect(dockComm, &QDockWidget::visibilityChanged, this, [this](bool visible){
        actionSerial->blockSignals(true);
        actionSerial->setChecked(visible);
        actionSerial->blockSignals(false);
        if(visible){
            dockComm->show();
        } else{
            dockComm->hide();
        }
    });


    connect(ui->pushButtonInit, &QPushButton::toggled, this, [this](bool on){
        if(on){
            QString path = "/opt/Basler/FramegrabberSDK/Hardware Applets/mE5-MA-VCL";
            bool applet = grabber->loadApplet(path+"/GaiaVision_PRJ_V2_Linux_AMD64.hap");
            bool conf = false;
            if(applet) conf = grabber->loadConfiguration(path+"/GaiaVision.mcf", true);

            if(conf) qDebug() << "Framegrabber is ready.";
            else qDebug() << "Init failed. Framegrabber is not set.";
            comm->connectSerial();

            ui->comboBoxExposureMode->setCurrentText(comm->getExposureMode());
            ui->spinBoxFrames->setValue(comm->getFrameCount()-1);
            ui->checkBoxCorrection->setChecked(grabber->getParameterIntValue("Device1_Process0_Parameter_ShadingEnable", 0));

            connect(ui->comboBoxExposureMode, &QComboBox::currentTextChanged, comm, &CameraCommunication::setExposureMode);
            connect(ui->spinBoxFrames, &QSpinBox::editingFinished, comm, [this]{
                comm->setFrameCount(ui->spinBoxFrames->value()+1);
            });
            connect(ui->doubleSpinBoxExp, &QDoubleSpinBox::editingFinished, comm, [this]{
                comm->setExposureTime(ui->doubleSpinBoxExp->value());
            });
            connect(ui->checkBoxCorrection, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state){
                if(state == Qt::CheckState::Checked){
                    grabber->setParameterValue("Device1_Process0_Parameter_ShadingEnable", 1);
                }else{
                    grabber->setParameterValue("Device1_Process0_Parameter_ShadingEnable", 0);
                }
            });
            connect(comm, &CameraCommunication::refreshed, this, [this]{
                ui->comboBoxExposureMode->setCurrentText(comm->getExposureMode());
                ui->spinBoxFrames->setValue(comm->getFrameCount()-1);
                ui->doubleSpinBoxExp->setValue(comm->getExposureTime());
            });

        }else{
            grabber->release();
            comm->disconnectSerial();

            disconnect(ui->comboBoxExposureMode, &QComboBox::currentTextChanged, nullptr, nullptr);
            disconnect(ui->spinBoxFrames, &QSpinBox::editingFinished, nullptr, nullptr);
            disconnect(ui->doubleSpinBoxExp, &QDoubleSpinBox::editingFinished, nullptr, nullptr);
            disconnect(ui->checkBoxCorrection, &QCheckBox::checkStateChanged, nullptr, nullptr);
            disconnect(comm, &CameraCommunication::refreshed, nullptr, nullptr);

            ui->spinBoxFrames->setValue(0);
            ui->doubleSpinBoxExp->setValue(0.);
            ui->comboBoxExposureMode->setCurrentText("Unknown");
            ui->checkBoxCorrection->setChecked(false);
        }
        ui->comboBoxExposureMode->setEnabled(on);
        ui->spinBoxFrames->setEnabled(on);
        ui->checkBoxCorrection->setEnabled(on);
        ui->doubleSpinBoxExp->setEnabled(on);
    });
    cal->setComm(comm);
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
    });
    connect(grabber, &Qylon::Grabber::grabbingState, this, [this](bool isGrabbing){
        actionStop->setEnabled(isGrabbing);
        actionStart->setEnabled(!isGrabbing);
    });
    connect(grabber, &Qylon::Grabber::released, this, [this]{
        actionStart->setEnabled(false);
    });
    connect(grabber, &Qylon::Grabber::updatedParametersValue, this, [this]{
        ui->checkBoxCorrection->setChecked(grabber->getParameterIntValue("Device1_Process0_Parameter_ShadingEnable", 0));
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
