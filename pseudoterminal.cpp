#include "pseudoterminal.h"

#include "pty.h"

#include <QtCore/QDebug>

PseudoTerminal::PseudoTerminal(QObject *parent)
    : QProcess(parent)
    , m_readMaster(0)
    , m_writeMaster(0)
    , m_readSlave(0)
    , m_writeSlave(0)
    , m_isOpen(false)
{
    setProcessChannelMode(QProcess::SeparateChannels);
    m_pty = new Pty();
}

PseudoTerminal::~PseudoTerminal()
{
    exit();
    closePty();
    delete m_pty;
    m_pty = 0;
}

void PseudoTerminal::startPty(const QString &program, const QStringList &arguments)
{
    openPty();
    connect(this, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(processStateChanged(QProcess::ProcessState)));
    connect(this, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(encounteredError(QProcess::ProcessError)));
    connect(this, SIGNAL(readyReadStandardError()),
            this, SLOT(readStandardError()));
    connect(this, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readStandardOutput()));
    setReadChannel(QProcess::StandardOutput);

    start(program, arguments);

    waitForStarted();
    Q_ASSERT(state() == QProcess::Running);
}

void PseudoTerminal::setupChildProcess()
{
    //We've forked, but haven't executed yet
    m_pty->setupChildProcess();
    m_pty->login();
}

void PseudoTerminal::processStateChanged(QProcess::ProcessState state)
{
    if (state == QProcess::NotRunning)
        m_pty->logout();
}

void PseudoTerminal::encounteredError(QProcess::ProcessError error)
{
    qDebug() << "encounteredError" << error;
}

void PseudoTerminal::readStandardError()
{
    qDebug() << "readyReadStandardError"
             << readAllStandardError();
}

void PseudoTerminal::readStandardOutput()
{
    qDebug() << "readyReadStandardOutput"
             << readAllStandardOutput();
}

void PseudoTerminal::readMaster()
{
    m_pty->readMaster();
}

void PseudoTerminal::writeMaster()
{
    m_pty->writeMaster();
}

void PseudoTerminal::readSlave()
{
    m_pty->readSlave();
}

void PseudoTerminal::writeSlave()
{
    m_pty->writeSlave();
}

bool PseudoTerminal::openPty()
{
    if (m_isOpen)
        return true;

    m_isOpen = m_pty->openPty();

    if (m_isOpen) {
        m_readMaster = new QSocketNotifier(m_pty->master(), QSocketNotifier::Read, this);
        m_readMaster->setEnabled(true);
        connect(m_readMaster, SIGNAL(activated(int)), this, SLOT(readMaster()));

        m_writeMaster = new QSocketNotifier(m_pty->master(), QSocketNotifier::Write, this);
        m_writeMaster->setEnabled(true);
        connect(m_writeMaster, SIGNAL(activated(int)), this, SLOT(writeMaster()));

        m_readSlave = new QSocketNotifier(m_pty->slave(), QSocketNotifier::Read, this);
        m_readSlave->setEnabled(true);
        connect(m_readSlave, SIGNAL(activated(int)), this, SLOT(readSlave()));

        m_writeSlave = new QSocketNotifier(m_pty->slave(), QSocketNotifier::Write, this);
        m_writeSlave->setEnabled(true);
        connect(m_writeSlave, SIGNAL(activated(int)), this, SLOT(writeSlave()));
    }

    return m_isOpen;
}

void PseudoTerminal::closePty()
{
    m_pty->closePty();
    m_isOpen = false;
}

void PseudoTerminal::login()
{
    m_pty->login();
}

void PseudoTerminal::logout()
{
    m_pty->logout();
}

void PseudoTerminal::exit()
{
    // FIXME should send exit to the process
}
