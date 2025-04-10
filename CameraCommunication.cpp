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
    connect(ui->pushButtonInit, &QPushButton::toggled, this, [this](bool toggle){
        try{
            if(toggle){
                std::string string("/opt/Basler/FramegrabberSDK/lib64/libclsersis.so");
                loadLibrary(string);
                int fw0=getValue(ADDR_FWVER0);
                int fw1=getValue(ADDR_FWVER1);
                print("Version:" + QString::number(fw1) + " Revision:" + QString::number(fw0), true);
                if(!initInterface()){
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
                ui->spinBoxWidth->setEnabled(false);
                ui->spinBoxHeight->setEnabled(false);
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

bool CameraCommunication::initInterface(){
    /// Get trigMode ("R0000\r"), set trigMode ("W0000")

    int trigger = getValue(ADDR_CNTRL0);
    ui->comboBoxTriggerMode->addItem("25 FPS", VALUE_TRIG_MODE_25); // 16
    ui->comboBoxTriggerMode->addItem("Sequence", VALUE_TRIG_MODE_SEQ); // 48
    ui->comboBoxTriggerMode->addItem("Trigger", VALUE_TRIG_MODE_TRIG); // 64
    ui->comboBoxTriggerMode->addItem("X FPS", VALUE_TRIG_MODE_XFPS); // 128
    switch(trigger){
    case VALUE_TRIG_MODE_25: ui->comboBoxTriggerMode->setCurrentText("25 FPS"); break;
    case VALUE_TRIG_MODE_SEQ: ui->comboBoxTriggerMode->setCurrentText("Sequence"); break;
    case VALUE_TRIG_MODE_TRIG: ui->comboBoxTriggerMode->setCurrentText("Trigger"); break;
    case VALUE_TRIG_MODE_XFPS: ui->comboBoxTriggerMode->setCurrentText("X FPS"); break;
    default: return false;
    }
    disconnect(ui->comboBoxTriggerMode, &QComboBox::currentTextChanged, nullptr, nullptr);
    connect(ui->comboBoxTriggerMode, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxTriggerMode->findText(value);
        int val = ui->comboBoxTriggerMode->itemData(idx).toInt();
        setValue(ADDR_CNTRL0, val);
    });
    ui->comboBoxTriggerMode->setEnabled(true);

    /// FULL WELL
    int fullWell = getValue(ADDR_SET0);
    ui->comboBoxFullWell->addItem("Low", VALUE_FULLWELL_LOW); // 0
    ui->comboBoxFullWell->addItem("High", VALUE_FULLWELL_HIGH); // 4
    switch(fullWell){
    case VALUE_FULLWELL_LOW: ui->comboBoxFullWell->setCurrentText("Low"); break;
    case VALUE_FULLWELL_HIGH: ui->comboBoxFullWell->setCurrentText("High"); break;
    default: return false;
    }
    disconnect(ui->comboBoxFullWell, &QComboBox::currentTextChanged, nullptr, nullptr);
    connect(ui->comboBoxFullWell, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxFullWell->findText(value);
        int val = ui->comboBoxFullWell->itemData(idx).toInt();
        setValue(ADDR_SET0, val);
    });
    ui->comboBoxFullWell->setEnabled(true);

    /// BINNING
    int binning = getValue(ADDR_BINNING);
    ui->comboBoxBinning->addItem("1x1", VALUE_BINNING_1X1); // 257
    ui->comboBoxBinning->addItem("2x2", VALUE_BINNING_2X2); // 514
    ui->comboBoxBinning->addItem("4x4", VALUE_BINNING_4X4); // 1028
    switch(binning){
    case VALUE_BINNING_1X1: ui->comboBoxBinning->setCurrentText("1x1"); break;
    case VALUE_BINNING_2X2: ui->comboBoxBinning->setCurrentText("2x2"); break;
    case VALUE_BINNING_4X4: ui->comboBoxBinning->setCurrentText("4x4"); break;
    default: return false;
    }
    disconnect(ui->comboBoxBinning, &QComboBox::currentTextChanged, nullptr, nullptr);
    connect(ui->comboBoxBinning, &QComboBox::currentTextChanged, this, [this](QString value){
        int idx = ui->comboBoxBinning->findText(value);
        int val = ui->comboBoxBinning->itemData(idx).toInt();
        setValue(ADDR_BINNING, val);
    });
    ui->comboBoxBinning->setEnabled(true);


    /// EXPOSURE TIME
    int exposureTimeLow = getValue(ADDR_TEXPLSB);
    if(exposureTimeLow == -1) return false;
    int exposureTimeHigh= getValue(ADDR_TEXPMSB);
    if(exposureTimeHigh == -1) return false;
    int readoutLow = getValue(ADDR_TREADOUT_LSB);
    if(readoutLow == -1) return false;
    int readoutHigh = getValue(ADDR_TREADOUT_MSB);
    if(readoutHigh == -1) return false;

    int exposureTime = (exposureTimeHigh << 16) | exposureTimeLow;
    int readoutTime = (readoutHigh << 16) | readoutLow;
    double corExposureTime = (exposureTime + readoutTime) * 0.01;
    ui->doubleSpinBoxExposureTime->setValue(corExposureTime);

    connect(ui->doubleSpinBoxExposureTime, &QDoubleSpinBox::valueChanged, this, [this](double value){
        int exposureTime = (int)(value * 100);

        int high = (exposureTime >> 16) & 0xFFFF;
        int low = exposureTime & 0xFFFF;
        setValue(ADDR_TEXPMSB, high);
        setValue(ADDR_TEXPLSB, low);
    });
    ui->doubleSpinBoxExposureTime->setEnabled(true);


    /// ROI
    int y = getValue(ADDR_ROI_Y);
    if(y==-1) return false;
    ui->spinBoxY->setValue(y);
    connect(ui->spinBoxY, &QSpinBox::valueChanged, this, [this](int value){
        setValue(ADDR_ROI_Y, value);
    });
    ui->spinBoxY->setEnabled(true);

    int h = getValue(ADDR_ROI_H);
    if(h==-1) return false;
    ui->spinBoxH->setValue(h);
    connect(ui->spinBoxH, &QSpinBox::valueChanged, this, [this](int value){
        setValue(ADDR_ROI_H, value);
    });
    ui->spinBoxH->setEnabled(true);

    int width = getValue(ADDR_IMG_WIDTH);
    if(width==-1) return false;
    ui->spinBoxWidth->setValue(width);
    connect(ui->spinBoxWidth, &QSpinBox::valueChanged, this, [this](int value){
        setValue(ADDR_IMG_WIDTH, value);
    });
    ui->spinBoxWidth->setEnabled(true);

    int height = getValue(ADDR_IMG_HEIGHT);
    if(height==-1) return false;
    ui->spinBoxHeight->setValue(height);
    connect(ui->spinBoxHeight, &QSpinBox::valueChanged, this, [this](int value){
        setValue(ADDR_IMG_HEIGHT, value);
    });
    ui->spinBoxHeight->setEnabled(true);

    print("Initialization finished.");
    return true;
}

void CameraCommunication::loadLibrary(std::string &libPath)
{
    print("Loading a communication library...");
    void *handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!handle) {
        throw std::runtime_error("Failed to load library: " + libPath);
        print("Failed to load a library. Please check the library. " + QString::fromStdString(libPath), 2);
    }
    try {
        auto error = clSerialInit(0, &serialRefPtr);
        if(error==0){
            clSetBaudRate(serialRefPtr, CL_BAUDRATE_115200);
            print("Library loaded.");
        }else{
            print("Library loading failed. Please check the serial status or libraries.", 2);
            // Init error
        }
    } catch (const std::exception& e) {
        print("Error occurred. " + QString::fromStdString(e.what()), 2);
    } catch (...){
        print("Error occurred with an unknown error.", 2);
    }
}

void CameraCommunication::debugUpdate(QString message)
{
    ui->textEditDebug->append(message);
}

int CameraCommunication::getValue(const char* addr){
    // Read command : R0 + address(000~999) + <CR>
    // Read response : S + 00000~99999 + <CR>

    QByteArray buffer = QString("R0" + QString(addr) + "\r").toUtf8();
    unsigned int numBytes = buffer.size();
    int mode = clSerialWrite(serialRefPtr, buffer.data(), &numBytes, 500);
    print("Sent command:" + QString::fromUtf8(buffer).remove("\r") + " Code:" + QString::number(mode) + (mode ? "(failure)" : "(success)") );

    if (mode == CL_ERR_NO_ERR) {
        QByteArray receiveBuf("S00000\r");
        unsigned int bufBytes = receiveBuf.size();
        int result = clSerialRead(serialRefPtr, receiveBuf.data(), &bufBytes, 500);
        if (result == CL_ERR_NO_ERR) {
            print("Recieved command:" + QString::fromUtf8(receiveBuf).remove("\r") + " Code:" + QString::number(result) + (result ? "(failure)" : "(success)") );

            if(receiveBuf.startsWith('S') && receiveBuf.endsWith('\r')){
                QString valueStr = receiveBuf.mid(1, receiveBuf.size() - 2);
                bool ok;
                int extractedValue = valueStr.toInt(&ok);
                if(ok){
                    print("Extracted value:" + QString::number(extractedValue));
                    return extractedValue;
                }
            }else{
                print("Received data format is inappropriate.");
            }
        }else{
            print("Failed to receive data(Command:" + QString::fromLocal8Bit(addr)+").", 2);
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
    print("Sent command:" + QString::fromUtf8(buffer).remove("\r") + " Code:" + QString::number(mode) + (mode ? "(failure)" : "(success)") );
    if(mode == CL_ERR_NO_ERR){
        QByteArray receiveBuf("X\r");
        unsigned int bufBytes = receiveBuf.size();
        int result = clSerialRead(serialRefPtr, receiveBuf.data(), &bufBytes, 500);
        if(result == CL_ERR_NO_ERR){
            print("Recieved command:" + QString::fromUtf8(receiveBuf).remove("\r") + " Code:" + QString::number(result) + (result ? "(failure)" : "(success)") );
            if(receiveBuf == "X\r"){
                print("Writing succeeded.");
                return true;
            }else{
                print("Failed to write data to the address(Command:" + QString::fromLocal8Bit(addr)+").", 2);
                return false;
            }
        }else{
            print("Failed to receive data(Command:" + QString::fromLocal8Bit(addr)+").", 2);
        }
    }
    return false;
}

void CameraCommunication::print(QString text, int type)
{
    if(type==0){
        ui->textEditDebug->setTextColor(Qt::black);
    }else if(type==1){ // info
        ui->textEditDebug->setTextColor(Qt::darkGreen);
    }else{
        ui->textEditDebug->setTextColor(Qt::darkRed);
    }
    ui->textEditDebug->append("["+QTime::currentTime().toString() +"] " + text);
}


