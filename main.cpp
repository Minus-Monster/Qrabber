#include "mainwindow.h"

#include <QApplication>
#include "Qylon.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(Resources);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    Qylon::Qylon q;
    w.setGrabber(q.addGrabber());

    return a.exec();
}
