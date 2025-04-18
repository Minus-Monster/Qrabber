#include "CalibrationDirectoryDialog.h"
#include "ui_CalibrationDirectoryDialog.h"
#include <QFileDialog>
#include <QMessageBox>

CalibrationDirectoryDialog::CalibrationDirectoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CalibrationDirectoryDialog)
{
    ui->setupUi(this);
    ui->lineEditDirBright->setText(QDir::homePath() + "/Bright");
    ui->lineEditDirDark->setText(QDir::homePath() + "/Dark");

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
        QDir dir(ui->lineEditDirBright->text());

        if(!dir.exists()){
            if(dir.mkpath("."));
            else qDebug() << "Failed to create " + ui->lineEditDirBright->text() + " directory.";
        }
        grabber->saveImage(ui->lineEditDirBright->text(), ui->lineEditNameBright->text(), ui->spinBoxBrightCnt->value());
    });
    connect(ui->pushButtonDarkStart, &QPushButton::clicked, this, [this]{
        QDir dir(ui->lineEditDirDark->text());

        if(!dir.exists()){
            if(dir.mkpath("."));
            else qDebug() << "Failed to create " + ui->lineEditDirDark->text() + " directory.";
        }
        grabber->saveImage(ui->lineEditDirDark->text(), ui->lineEditNameDark->text(), ui->spinBoxDarkCnt->value());
    });
    connect(ui->pushButtonAverage, &QPushButton::clicked, this, [this]{
        bool ok1=generateAverageImages(ui->lineEditDirBright->text(), ui->lineEditNameBright->text(), ui->spinBoxBrightCnt->value());
        bool ok2=generateAverageImages(ui->lineEditDirDark->text(), ui->lineEditNameDark->text(), ui->spinBoxDarkCnt->value());
        if(ok1&&ok2){
            QMessageBox::information(this, this->windowTitle(), "Generated average images successfully.");
        }else{
            QMessageBox::warning(this, this->windowTitle(), "Failed to generate average images. \nPlease check the images that want to create the average image.");
        }
    });
}

bool CalibrationDirectoryDialog::generateAverageImages(QString savePath, QString saveName, int numFrames)
{
    QStringList filters(QStringList() << "*[0-9].tiff");
    QDir path(savePath);
    QStringList fileList = path.entryList(filters, QDir::Files);
    std::sort(fileList.begin(), fileList.end(), [](const QString &a, const QString &b) {
        QRegularExpression re("(\\d+)");
        QRegularExpressionMatch matchA = re.match(a);
        QRegularExpressionMatch matchB = re.match(b);
        int numA = matchA.hasMatch() ? matchA.captured(0).toInt() : 0;
        int numB = matchB.hasMatch() ? matchB.captured(0).toInt() : 0;
        return numA < numB;
    });

    int count = qMin(numFrames, fileList.size());
    if (count == 0) return false;

    int width = 0, height = 0, bpp = 0;
    bool valid = true;

    std::vector<QString> validFiles;

    // First pass: check dimensions and format
    for (int i = 0; i < count; ++i) {
        SisoIoImageEngine *handle;
        QString filePath = path.absoluteFilePath(fileList.at(i));
        if (IoImageOpen(filePath.toStdString().c_str(), &handle) != 0) continue;

        if (i == 0) {
            width = IoGetWidth(handle);
            height = IoGetHeight(handle);
            bpp = IoGetBitsPerPixel(handle);
            if (bpp != 16) {
                qWarning() << "Only Grayscale16 supported.";
                IoFreeImage(handle);
                return false;
            }
        } else {
            if (IoGetWidth(handle) != width || IoGetHeight(handle) != height || IoGetBitsPerPixel(handle) != bpp) {
                qWarning() << "Image format or size mismatch for:" << fileList.at(i);
                IoFreeImage(handle);
                valid = false;
                break;
            }
        }
        validFiles.push_back(filePath);
        IoFreeImage(handle);
    }

    if (!valid || validFiles.empty()) return false;

    std::vector<double> sumPixels(width * height, 0);

    // Second pass: accumulate
    for (const auto &filePath : validFiles) {
        SisoIoImageEngine *handle;
        if (IoImageOpen(filePath.toStdString().c_str(), &handle) != 0) continue;

        const quint16* data = reinterpret_cast<const quint16*>(IoImageGetData(handle));
        for (int i = 0; i < width * height; ++i) {
            sumPixels[i] += data[i];
        }
        IoFreeImage(handle);
    }

    // Prepare final images
    std::vector<quint16> avgImage(width * height);
    std::vector<uchar> upperImage(width * height);
    std::vector<uchar> lowerImage(width * height);

    for (int i = 0; i < width * height; ++i) {
        quint16 avgVal = static_cast<quint16>(sumPixels[i] / count);
        avgImage[i] = avgVal;
        upperImage[i] = static_cast<uchar>(avgVal >> 8);
        lowerImage[i] = static_cast<uchar>(avgVal & 0xFF);
    }

    auto saveRawImage = [&](void* data, int bit, QString outPath) -> bool {
        return IoWriteTiff(outPath.toStdString().c_str(), (unsigned char*)data, width, height, bit, 1) == FG_OK;
    };

    QString baseName = QFileInfo(saveName).completeBaseName();
    QString outputFilePath = savePath + "/Average_" + baseName + ".tiff";
    QString outputUpperFilePath = savePath + "/Upper_" + baseName + ".tiff";
    QString outputLowerFilePath = savePath + "/Lower_" + baseName + ".tiff";

    if (saveRawImage(avgImage.data(), 16, outputFilePath) &&
        saveRawImage(upperImage.data(), 8, outputUpperFilePath) &&
        saveRawImage(lowerImage.data(), 8, outputLowerFilePath)) {
        qDebug() << "Saved:" << outputFilePath << outputUpperFilePath << outputLowerFilePath;
    } else {
        qWarning() << "Failed to save one or more images.";
        return false;
    }
    return true;
}
