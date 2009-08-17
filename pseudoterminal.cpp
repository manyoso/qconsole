#include "pseudoterminal.h"

#include <QtCore/QDebug>

#if defined(Q_OS_LINUX)
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <utmpx.h>
#include <termios.h>
#endif

PseudoTerminal::PseudoTerminal(QObject *parent)
    : QProcess(parent)
    , m_master(-1)
    , m_slave(-1)
    , m_isOpen(false)
{
}

PseudoTerminal::~PseudoTerminal()
{
    exit();
    closePty();
}

void PseudoTerminal::startPty(const QString &program, const QStringList &arguments)
{
    openPty();
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
    dup2(m_slave, STDIN_FILENO);
    dup2(m_slave, STDOUT_FILENO);
    dup2(m_slave, STDERR_FILENO);
    login();
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
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "readMaster" << data << len;
}

void PseudoTerminal::writeMaster()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_master, bytes.constData(), bytes.size());
    //qDebug() << "writeMaster";
}

void PseudoTerminal::readSlave()
{
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "readSlave" << data << len;
}

void PseudoTerminal::writeSlave()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_slave, bytes.constData(), bytes.size());
    //qDebug() << "writeSlave";
}

bool PseudoTerminal::openPty()
{
    if (m_isOpen)
        return false;

#if defined(Q_OS_LINUX)
    // Open master pseudo terminal
    m_master = ::posix_openpt(O_RDWR | O_NOCTTY);

    Q_ASSERT(m_master > 0);

    // Get the associated slave name
    m_slaveName = ptsname(m_master);

    // Change the mode and ownership of the slave associated to point to this process
    Q_ASSERT(!grantpt(m_master));

    // Unlock the slave associated with this master pseudo terminal
    unlockpt(m_master);

    // Open the slave
    m_slave = ::open(m_slaveName, O_RDWR | O_NOCTTY);
    if (m_slave < 0) {
        qWarning() << "Warning: Can't open slave pseudo terminal!";
        ::close(m_master);
        m_master = -1;
        m_isOpen = false;
        return false;
    }

    fcntl(m_master, F_SETFD, FD_CLOEXEC);
    fcntl(m_slave, F_SETFD, FD_CLOEXEC);

    struct ::termios ttmode;
    Q_ASSERT(!tcgetattr(m_master, &ttmode));
    ttmode.c_iflag |= IUTF8;
    Q_ASSERT(!tcsetattr(m_master, TCSANOW, &ttmode));

    m_readMaster = new QSocketNotifier(m_master, QSocketNotifier::Read, this);
    m_readMaster->setEnabled(true);
    connect(m_readMaster, SIGNAL(activated(int)), this, SLOT(readMaster()));

    m_writeMaster = new QSocketNotifier(m_master, QSocketNotifier::Write, this);
    m_writeMaster->setEnabled(true);
    connect(m_writeMaster, SIGNAL(activated(int)), this, SLOT(writeMaster()));

    m_readSlave = new QSocketNotifier(1, QSocketNotifier::Read, this);
    m_readSlave->setEnabled(true);
    connect(m_readSlave, SIGNAL(activated(int)), this, SLOT(readSlave()));

    m_writeSlave = new QSocketNotifier(1, QSocketNotifier::Write, this);
    m_writeSlave->setEnabled(true);
    connect(m_writeSlave, SIGNAL(activated(int)), this, SLOT(writeSlave()));

    qDebug() << "Successfully opened" << m_slaveName;
#elif defined(Q_OS_WIN)
    m_isOpen = false;
    return false;
#else
    m_isOpen = false;
    return false;
#endif

    m_isOpen = true;
    return true;
}

void PseudoTerminal::closePty()
{
#if defined(Q_OS_LINUX)
    if (m_slave >= 0) {
        ::close(m_slave);
        m_slave = -1;
    }
    if (m_master >= 0) {
        ::close(m_master);
        m_master = -1;
    }
#elif defined(Q_OS_WIN)
#endif

    m_isOpen = false;
}

void PseudoTerminal::login()
{
#if defined(Q_OS_LINUX)
    // Create a new session associated with this process
    pid_t pid = setsid();

    // Make the slave the controlling pseudo terminal
    ioctl(m_slave, TIOCSCTTY, 0);

    // Set the foreground process id to the new group
    tcsetpgrp(m_slave, pid);

    struct utmpx loginInfo;
    memset(&loginInfo, 0, sizeof(loginInfo));

    strncpy(loginInfo.ut_user, qgetenv("USER"), sizeof(loginInfo.ut_user));

    QString device = m_slaveName;
    if (device.startsWith("/dev/"))
        device = device.mid(5);
    const char *d = device.toLatin1().constData();
    strncpy(loginInfo.ut_line, d, sizeof(loginInfo.ut_line));
    strncpy(loginInfo.ut_id, d + strlen(d) - sizeof(loginInfo.ut_id), sizeof(loginInfo.ut_id));
    gettimeofday(&loginInfo.ut_tv, 0);

    loginInfo.ut_type = USER_PROCESS;
    loginInfo.ut_pid = getpid();

    utmpxname(_PATH_UTMPX);
    setutxent();
    pututxline(&loginInfo);
    endutxent();
    updwtmpx(_PATH_WTMPX, &loginInfo);
#elif defined(Q_OS_WIN)
#endif
}

void PseudoTerminal::logout()
{
#if defined(Q_OS_LINUX)
#elif defined(Q_OS_WIN)
#endif
}

void PseudoTerminal::exit()
{
    // FIXME should send exit to the process
    Q_ASSERT(state() == QProcess::Running);
    kill();
    waitForFinished(0);
    Q_ASSERT(state() == QProcess::NotRunning);
}
