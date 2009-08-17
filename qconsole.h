#ifndef QCONSOLE_H
#define QCONSOLE_H

#include <QtGui/QWidget>

class PseudoTerminal;

class QConsole : public QWidget
{
    Q_OBJECT

public:
    QConsole(QWidget *parent = 0);
    ~QConsole();

private:
    PseudoTerminal *m_pty;
};

#endif // QCONSOLE_H
