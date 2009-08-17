#include "qconsole.h"

#include "pseudoterminal.h"

#include <QtCore/QDebug>

QConsole::QConsole(QWidget *parent)
    : QWidget(parent)
{
    m_pty = new PseudoTerminal(this);
    m_pty->startPty("/bin/bash", QStringList());
}

QConsole::~QConsole()
{
}
