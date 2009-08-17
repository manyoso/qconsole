#include <QtGui/QApplication>
#include "qconsole.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QConsole w;
    w.show();
    return a.exec();
}
