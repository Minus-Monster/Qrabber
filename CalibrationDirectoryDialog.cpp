#include "CalibrationDirectoryDialog.h"
#include "ui_CalibrationDirectoryDialog.h"
#include <QFileDialog>

CalibrationDirectoryDialog::CalibrationDirectoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CalibrationDirectoryDialog)
{
    ui->setupUi(this);
    connect(ui->pushButtonBright, &QPushButton::clicked, this, [this]{
        auto path = QFileDialog::getExistingDirectory(this, "Set Bright Calibration Folder");
        ui->lineEditDirBright->setText(path);
    });
    connect(ui->pushButtonDark, &QPushButton::clicked, this, [this]{
        auto path = QFileDialog::getExistingDirectory(this, "Set Dark Calibration Folder");
        ui->lineEditDirDark->setText(path);
    });

}

CalibrationDirectoryDialog::~CalibrationDirectoryDialog()
{
    delete ui;
}

void CalibrationDirectoryDialog::setGrabber(Qylon::Grabber *g){
    grabber = g;
    connect(grabber, &Qylon::Grabber::loadedApplet, this, [this]{
        ui->pushButtonBrightStart->setEnabled(true);
        ui->pushButtonDarkStart->setEnabled(true);
    });
    connect(grabber, &Qylon::Grabber::grabbingState, this, [this](bool on){
        ui->pushButtonBrightStart->setEnabled(!on);
        ui->pushButtonDarkStart->setEnabled(!on);
    });
    connect(ui->pushButtonBrightStart, &QPushButton::clicked, this, [this]{
        grabber->saveImage(ui->lineEditDirBright->text(), ui->lineEditNameBright->text(), ui->spinBoxBrightCnt->value());
    });
    connect(ui->pushButtonDarkStart, &QPushButton::clicked, this, [this]{
        grabber->saveImage(ui->lineEditDirDark->text(), ui->lineEditNameDark->text(), ui->spinBoxDarkCnt->value());
    });
}
