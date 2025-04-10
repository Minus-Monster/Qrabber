#ifndef CAMERACOMMUNICATION_H
#define CAMERACOMMUNICATION_H

#include <QDialog>

namespace Ui {
class CameraCommunication;
}

class CameraCommunication : public QDialog
{
    Q_OBJECT

public:
    explicit CameraCommunication(QWidget *parent = nullptr);
    ~CameraCommunication();

    void loadLibrary(std::string &libPath);

    void debugUpdate(QString message);

    int getValue(const char *addr);
    bool setValue(const char *addr, int value);

    void print(QString text, int type=0);

private:
    Ui::CameraCommunication *ui;
    void *serialRefPtr;

    bool initInterface();
};

#endif // CAMERACOMMUNICATION_H
