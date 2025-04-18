#ifndef CALIBRATIONDIRECTORYDIALOG_H
#define CALIBRATIONDIRECTORYDIALOG_H

#include <QDialog>
#include "Grabber.h"

namespace Ui {
class CalibrationDirectoryDialog;
}

class CalibrationDirectoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDirectoryDialog(QWidget *parent = nullptr);
    ~CalibrationDirectoryDialog();
    void setGrabber(Qylon::Grabber *g);
    bool generateAverageImages(QString savePath, QString saveName, int numFrames);

private:
    Ui::CalibrationDirectoryDialog *ui;
    Qylon::Grabber *grabber;
};

#endif // CALIBRATIONDIRECTORYDIALOG_H
