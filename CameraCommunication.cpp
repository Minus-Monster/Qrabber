#include "CameraCommunication.h"
#include "ui_CameraCommunication.h"
#include <dlfcn.h>
#include <iostream>
#include "clser.h"
#include <QTime>
#include <QMessageBox>

#define ADDR_CNTRL0 "000"
#define ADDR_SET0 "003" // FULL WELL is in [2]
#define ADDR_IMG_HEIGHT "068"
#define ADDR_IMG_WIDTH "069"
#define ADDR_ROI_Y "402"
#define ADDR_ROI_H "403"
#define ADDR_DDS "425"
#define ADDR_BINNING "440"
#define ADDR_NUM_FRAME_IN_SEQ "017"
#define ADDR_FWVER0 "129"
#define ADDR_FWVER1 "128"
#define ADDR_VERSION "127"

#define ADDR_TEXPMSB "011"
#define ADDR_TEXPLSB "012"
#define ADDR_TREADOUT_LSB "410"
#define ADDR_TREADOUT_MSB "411"

// TRIGGER MODE
#define VALUE_TRIG_MODE_25 16 //"0001"
#define VALUE_TRIG_MODE_SEQ 48 //"0011"
#define VALUE_TRIG_MODE_TRIG 64 //"0100"
#define VALUE_TRIG_MODE_XFPS 128 //"1000"

#define VALUE_FULLWELL_LOW 0
#define VALUE_FULLWELL_HIGH 4

#define VALUE_BINNING_1X1 257
#define VALUE_BINNING_2X2 514
#define VALUE_BINNING_4X4 1028



CameraCommunication::CameraCommunication(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CameraCommunication)
{
    ui->setupUi(this);
    initInterface();
    connect(ui->pushButtonInit, &QPushButton::toggled, this, [this](bool toggle){
        try{
            if(toggle){
                std::string string("/opt/Basler/FramegrabberSDK/lib64/libclsersis.so");
                if(loadLibrary(string)){
                    int fw0=getValue(ADDR_FWVER0);
                    int fw1=getValue(ADDR_FWVER1);
                    print("Version:" + QString::number(fw1) + " Revision:" + QString::number(fw0), true);
                    initInterface();

                    setValue(ADDR_CNTRL0, VALUE_TRIG_MODE_25);
                    setValue(ADDR_SET0, VALUE_FULLWELL_LOW);
                    setValue(ADDR_BINNING, VALUE_BINNING_1X1);

                    refreshAlldata();
                }else{
                    ui->pushButtonInit->setChecked(false);
                    QMessageBox::warning(this, "Detector Configuration", "Initialization is failed. Please check your camera connections.");
                }
            }else{
                ui->comboBoxBinning->setEnabled(false);
                ui->comboBoxFullWell->setEnabled(false);
                ui->comboBoxTriggerMode->setEnabled(false);
                ui->doubleSpinBoxExposureTime->setEnabled(false);
                ui->spinBoxH->setEnabled(false);
                ui->spinBoxY->setEnabled(false);
                ui->spinBox_seqFrame->setEnabled(false);
                ui->spinBoxWidth->setEnabled(false);
                ui->spinBoxHeight->setEnabled(false);
                clearAll();
            }
        }catch(std::runtime_error &e){
            std::cerr << e.what() << std::endl;
        }
    });
}

CameraCommunication::~CameraCommunication()
{
    delete ui;
}

bool CameraCommunication::connectSerial()
{
    ui->pushButtonInit->setChecked(true);

    return true;
}

void CameraCommunication::disconnectSerial()
{
    ui->pushButtonInit->setChecked(false);
}

bool CameraCommunication::initInterface(){
    /// Get trigMode ("R0000\r"), set trigMode ("W0000")
    ui->comboBoxTriggerMode->addItem("25 FPS", VALUE_TRIG_MODE_25); // 16
    ui->comboBoxTriggerMode->addItem("Sequence", VALUE_TRIG_MODE_SEQ); // 48
    ui->comboBoxTriggerMode->addItem("Trigger", VALUE_TRIG_MODE_TRIG); // 64
    ui->comboBoxTriggerMode->addItem("X FPS", VALUE_TRIG_MODE_XFPS); // 128
    ui->comboBoxTriggerMode->addItem("Unknown", 0);
    connect(ui->comboBoxTriggerMode, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxTriggerMode->findText(value);
        int val = ui->comboBoxTriggerMode->itemData(idx).toInt();
        bool isSequence = (ui->comboBoxTriggerMode->currentText() == "Sequence");
        ui->spinBox_seqFrame->setEnabled(isSequence);

        setValue(ADDR_CNTRL0, val);
        refreshAlldata();
    });
    ui->comboBoxTriggerMode->setEnabled(true);

    ui->spinBox_seqFrame->setRange(0, 9999);
    connect(ui->spinBox_seqFrame, &QSpinBox::editingFinished, this, [this]{
        setValue(ADDR_NUM_FRAME_IN_SEQ, ui->spinBox_seqFrame->value());
        refreshAlldata();
    });

    /// FULL WELL
    ui->comboBoxFullWell->addItem("Low", VALUE_FULLWELL_LOW); // 0
    ui->comboBoxFullWell->addItem("High", VALUE_FULLWELL_HIGH); // 4
    ui->comboBoxFullWell->addItem("Unknown", 0);
    connect(ui->comboBoxFullWell, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxFullWell->findText(value);
        int val = ui->comboBoxFullWell->itemData(idx).toInt();
        setValue(ADDR_SET0, val);
        refreshAlldata();
    });
    ui->comboBoxFullWell->setEnabled(true);

    /// BINNING
    ui->comboBoxBinning->addItem("1x1", VALUE_BINNING_1X1); // 257
    ui->comboBoxBinning->addItem("2x2", VALUE_BINNING_2X2); // 514
    ui->comboBoxBinning->addItem("4x4", VALUE_BINNING_4X4); // 1028
    ui->comboBoxBinning->addItem("Unknown", 0);
    connect(ui->comboBoxBinning, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxBinning->findText(value);
        int val = ui->comboBoxBinning->itemData(idx).toInt();
        setValue(ADDR_BINNING, val);
        refreshAlldata();
    });
    ui->comboBoxBinning->setEnabled(true);

    ui->doubleSpinBoxExposureTime->setValue(0);
    connect(ui->doubleSpinBoxExposureTime, &QDoubleSpinBox::editingFinished, this, [=](){
        int treadout_lsb = getValue(ADDR_TREADOUT_LSB);
        int treadout_msb = getValue(ADDR_TREADOUT_MSB);
        if (treadout_lsb < 0 || treadout_msb < 0) {
            print("Failed to read readout registers.", 3);
            return;
        }

        uint32_t readout_us = (treadout_msb << 16) | treadout_lsb;
        double readout_ms = readout_us / 1000.0;

        double exposureOnly_ms = ui->doubleSpinBoxExposureTime->value() - readout_ms;
        if (exposureOnly_ms < 0) exposureOnly_ms = 0;

        uint32_t regValue = static_cast<uint32_t>(exposureOnly_ms * 100); // 0.01ms 단위

        int high = (regValue >> 16) & 0xFFFF;
        int low = regValue & 0xFFFF;

        setValue(ADDR_TEXPMSB, high);
        setValue(ADDR_TEXPLSB, low);
        refreshAlldata();
    });
    ui->doubleSpinBoxExposureTime->setEnabled(true);


    /// ROI
    ui->spinBoxY->setValue(0);
    connect(ui->spinBoxY, &QSpinBox::editingFinished, this, [this](){

        setValue(ADDR_ROI_Y, ui->spinBoxY->value());
        refreshAlldata();
    });
    ui->spinBoxY->setEnabled(true);

    ui->spinBoxH->setValue(0);
    connect(ui->spinBoxH, &QSpinBox::editingFinished, this, [this](){
        if(ui->spinBoxH->value()==0) return;

        setValue(ADDR_ROI_H, ui->spinBoxH->value());
        refreshAlldata();
    });
    ui->spinBoxH->setEnabled(true);

    ui->spinBoxWidth->setValue(0);
    ui->spinBoxWidth->setEnabled(false);

    ui->spinBoxHeight->setValue(0);
    ui->spinBoxHeight->setEnabled(false);

    print("Initialization finished.");

    return true;
}

bool CameraCommunication::loadLibrary(std::string &libPath)
{
    print("Loading a communication library...");
    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!handle) {
        print("Failed to load a library. Please check the library. " + QString::fromStdString(libPath), 2);
        return false;
    }
    try {
        auto error = clSerialInit(0, &serialRefPtr);
        if(error==0){
            clSetBaudRate(serialRefPtr, CL_BAUDRATE_115200);
            print("Library loaded.");
            return true;
        }else{
            print("Library loading failed. Please check the serial status or libraries.", 2);
            // Init error
            return false;
        }
    } catch (const std::exception& e) {
        print("Error occurred. " + QString::fromStdString(e.what()), 2);
    } catch (...){
        print("Error occurred with an unknown error.", 2);
    }
    return false;
}

void CameraCommunication::print(QString message, int num)
{
    if(num==2) qWarning().noquote() << message;
    else qDebug().noquote() << message;
}

int CameraCommunication::getValue(const char* addr){
    // Read command : R0 + address(000~999) + <CR>
    // Read response : S + 00000~99999 + <CR>

    QByteArray buffer = QString("R0" + QString(addr) + "\r").toUtf8();
    unsigned int numBytes = buffer.size();
    int mode = clSerialWrite(serialRefPtr, buffer.data(), &numBytes, 500);
    print("Sent command:" + QString::fromUtf8(buffer).remove("\r") + " Code:" + QString::number(mode) + (mode ? "(failure)" : "(success)"),1 );

    if (mode == CL_ERR_NO_ERR) {
        QByteArray receiveBuf("S99999\r");
        unsigned int bufBytes = receiveBuf.size();
        int result = clSerialRead(serialRefPtr, receiveBuf.data(), &bufBytes, 500);
        if (result == CL_ERR_NO_ERR) {
            print("Recieved command:" + QString::fromUtf8(receiveBuf).remove("\r") + " Code:" + QString::number(result) + (result ? "(failure)" : "(success)"),1 );

            if(receiveBuf.startsWith('S') && receiveBuf.endsWith('\r')){
                QString valueStr = receiveBuf.mid(1, receiveBuf.size() - 2);
                bool ok;
                int extractedValue = valueStr.toInt(&ok);
                if(ok){
                    return extractedValue;
                }
            }else{
                print("Received data format is inappropriate.");
            }
        }else{
            print("Failed to receive data(Command:" + QString::fromLocal8Bit(addr)+").", 3);
        }
    }
    return -1;
}

bool CameraCommunication::setValue(const char* addr, int value)
{
    // Write command : W0 + address(000~999) + Data(00000~99999) + <CR>
    // Write response : X <CR>  -> correct
    //                  E <CR>  -> Incorrect

    QByteArray buffer = QString("W0" + QString(addr) + QString::number(value).rightJustified(5, '0') + "\r").toUtf8();
    unsigned int numBytes = buffer.size();
    int mode = clSerialWrite(serialRefPtr, buffer.data(), &numBytes, 500);
    print("Sent command:" + QString::fromUtf8(buffer).remove("\r") + " Code:" + QString::number(mode) + (mode ? "(failure)" : "(success)"),2 );
    if(mode == CL_ERR_NO_ERR){
        QByteArray receiveBuf("X\r");
        unsigned int bufBytes = receiveBuf.size();
        int result = clSerialRead(serialRefPtr, receiveBuf.data(), &bufBytes, 500);
        if(result == CL_ERR_NO_ERR){
            print("Recieved command:" + QString::fromUtf8(receiveBuf).remove("\r") + " Code:" + QString::number(result) + (result ? "(failure)" : "(success)"),2 );
            if(receiveBuf == "X\r"){
                print("Writing succeeded.");
                return true;
            }else{
                print("Failed to write data to the address(Command:" + QString::fromLocal8Bit(addr)+").", 3);
                return false;
            }
        }else{
            print("Failed to receive data(Command:" + QString::fromLocal8Bit(addr)+").", 3);
        }
    }
    return false;
}

void CameraCommunication::refreshAlldata()
{
    int trigger = getValue(ADDR_CNTRL0);
    ui->comboBoxTriggerMode->blockSignals(true);
    switch(trigger){
    case VALUE_TRIG_MODE_25:{
        ui->comboBoxTriggerMode->setCurrentText("25 FPS");
        ui->doubleSpinBoxExposureTime->setEnabled(false);
    }break;
    case VALUE_TRIG_MODE_SEQ:{
        ui->doubleSpinBoxExposureTime->setEnabled(true);
        ui->comboBoxTriggerMode->setCurrentText("Sequence");
    }break;
    case VALUE_TRIG_MODE_TRIG:{
        ui->comboBoxTriggerMode->setCurrentText("Trigger");
        ui->doubleSpinBoxExposureTime->setEnabled(true);
    }break;

    case VALUE_TRIG_MODE_XFPS:{
        ui->doubleSpinBoxExposureTime->setEnabled(true);
        ui->comboBoxTriggerMode->setCurrentText("X FPS");
    }break;
    default: ui->comboBoxTriggerMode->setCurrentText("Unknown");
    }
    ui->comboBoxTriggerMode->blockSignals(false);

    ui->spinBox_seqFrame->blockSignals(true);
    bool isSequence = (ui->comboBoxTriggerMode->currentText() == "Sequence");
    ui->spinBox_seqFrame->setEnabled(isSequence);
    ui->spinBox_seqFrame->setValue(getValue(ADDR_NUM_FRAME_IN_SEQ));
    ui->spinBox_seqFrame->blockSignals(false);

    int fullWell = getValue(ADDR_SET0);
    ui->comboBoxFullWell->blockSignals(true);
    switch(fullWell){
    case VALUE_FULLWELL_LOW: ui->comboBoxFullWell->setCurrentText("Low"); break;
    case VALUE_FULLWELL_HIGH: ui->comboBoxFullWell->setCurrentText("High"); break;
    default: ui->comboBoxFullWell->setCurrentText("Unknown");
    }
    ui->comboBoxFullWell->blockSignals(false);

    int binning = getValue(ADDR_BINNING);
    ui->comboBoxBinning->blockSignals(true);
    switch(binning){
    case VALUE_BINNING_1X1: ui->comboBoxBinning->setCurrentText("1x1"); break;
    case VALUE_BINNING_2X2: ui->comboBoxBinning->setCurrentText("2x2"); break;
    case VALUE_BINNING_4X4: ui->comboBoxBinning->setCurrentText("4x4"); break;
    default: ui->comboBoxBinning->setCurrentText("Unknown");
    }
    ui->comboBoxBinning->blockSignals(false);

    auto getExposureTime = [=]() -> double{
        int trigMode = getValue(ADDR_CNTRL0);
        if (trigMode == VALUE_TRIG_MODE_25) {
            return 40.0;
        }
        int msb = getValue(ADDR_TEXPMSB);
        int lsb = getValue(ADDR_TEXPLSB);
        int treadout_lsb = getValue(ADDR_TREADOUT_LSB);
        int treadout_msb = getValue(ADDR_TREADOUT_MSB);
        if (msb < 0 || lsb < 0 || treadout_lsb < 0 || treadout_msb < 0) return -1;

        uint32_t exposure = (msb << 16) | lsb;       // 0.01ms 단위
        uint32_t readout = (treadout_msb << 16) | treadout_lsb; // 1us 단위

        double exposure_ms = exposure / 100.0;
        double readout_ms = readout / 1000.0;

        return exposure_ms + readout_ms;
    };

    ui->doubleSpinBoxExposureTime->blockSignals(true);
    ui->doubleSpinBoxExposureTime->setValue(getExposureTime());
    ui->doubleSpinBoxExposureTime->blockSignals(false);

    ui->spinBoxH->blockSignals(true);
    ui->spinBoxH->setValue(getValue(ADDR_ROI_Y));
    ui->spinBoxH->blockSignals(false);

    ui->spinBoxY->blockSignals(true);
    ui->spinBoxY->setValue(getValue(ADDR_ROI_Y));
    ui->spinBoxY->blockSignals(false);

    ui->spinBoxWidth->blockSignals(true);
    ui->spinBoxWidth->setValue(getValue(ADDR_IMG_WIDTH));
    ui->spinBoxWidth->blockSignals(false);

    ui->spinBoxHeight->blockSignals(true);
    ui->spinBoxHeight->setValue(getValue(ADDR_IMG_HEIGHT));
    ui->spinBoxHeight->blockSignals(false);

    emit refreshed();
}

void CameraCommunication::clearAll()
{
    disconnect(ui->comboBoxTriggerMode, &QComboBox::currentTextChanged, nullptr, nullptr);
    disconnect(ui->comboBoxFullWell, &QComboBox::currentTextChanged, nullptr, nullptr);
    disconnect(ui->comboBoxBinning, &QComboBox::currentTextChanged, nullptr, nullptr);
    disconnect(ui->spinBoxWidth, &QSpinBox::editingFinished, nullptr, nullptr);
    disconnect(ui->spinBoxHeight, &QSpinBox::editingFinished, nullptr, nullptr);
    disconnect(ui->spinBoxY, &QSpinBox::editingFinished, nullptr, nullptr);
    disconnect(ui->spinBoxH, &QSpinBox::editingFinished, nullptr, nullptr);
    disconnect(ui->doubleSpinBoxExposureTime, &QDoubleSpinBox::editingFinished, nullptr, nullptr);


    ui->comboBoxTriggerMode->clear();
    ui->comboBoxBinning->clear();
    ui->comboBoxFullWell->clear();
    ui->spinBox_seqFrame->setValue(0);
    ui->spinBoxH->setValue(0);
    ui->spinBoxY->setValue(0);
    ui->spinBoxWidth->setValue(0);
    ui->spinBoxHeight->setValue(0);
    ui->doubleSpinBoxExposureTime->setValue(0);

}

double CameraCommunication::getExposureTime()
{
    return ui->doubleSpinBoxExposureTime->value();
}

QString CameraCommunication::getExposureMode()
{
    return ui->comboBoxTriggerMode->currentText();
}

int CameraCommunication::getFrameCount()
{
    return ui->spinBox_seqFrame->value();
}

void CameraCommunication::setExposureMode(QString mode)
{
    ui->comboBoxTriggerMode->setCurrentText(mode);
}

void CameraCommunication::setFrameCount(int frame)
{
    ui->spinBox_seqFrame->setValue(frame);
}

void CameraCommunication::setExposureTime(double time)
{
    ui->doubleSpinBoxExposureTime->setValue(time);
    emit ui->doubleSpinBoxExposureTime->editingFinished();
}


