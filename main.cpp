#include "mainwindow.h"

#include <QApplication>
#include "Qylon.h"
#include <QStyleFactory>

MainWindow *window = nullptr;
void setDebugMessage(QtMsgType, const QMessageLogContext &context, const QString &msg)
{
    // qDebug() << context.file << context.line << context.function;
    QMetaObject::invokeMethod(window, "append", Qt::QueuedConnection, Q_ARG(QString, msg));
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(Resources);
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    window = &w;
    w.show();
    qRegisterMetaType<QTextCursor>("QTextCursor");
    qInstallMessageHandler(setDebugMessage);

    Qylon::Qylon q;
    w.setGrabber(q.addGrabber());

    return a.exec();
}
