#include "pty.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <utmpx.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

Pty::Pty()
    : m_master(-1)
    , m_slave(-1)
    , m_slaveName(0)
{
}

Pty::~Pty()
{
}

bool Pty::openPty()
{
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
        return false;
    }

    fcntl(m_master, F_SETFD, FD_CLOEXEC);
    fcntl(m_slave, F_SETFD, FD_CLOEXEC);

    struct ::termios ttmode;
    Q_ASSERT(!tcgetattr(m_master, &ttmode));
    ttmode.c_iflag |= IUTF8;
    Q_ASSERT(!tcsetattr(m_master, TCSANOW, &ttmode));

    qDebug() << "Successfully opened" << m_slaveName;

    return true;
}

void Pty::closePty()
{
    if (m_slave >= 0) {
        ::close(m_slave);
        m_slave = -1;
    }
    if (m_master >= 0) {
        ::close(m_master);
        m_master = -1;
    }
}

void Pty::login()
{
    // Create a new session associated with this process
    pid_t pid = setsid();

    // Make the slave the controlling pseudo terminal
    ioctl(m_slave, TIOCSCTTY, 0);

    // Set the foreground process id to the new group
    tcsetpgrp(m_slave, pid);

    struct utmpx loginInfo;
    memset(&loginInfo, 0, sizeof(loginInfo));

    strncpy(loginInfo.ut_user, qgetenv("USER"), sizeof(loginInfo.ut_user));

    QString device(m_slaveName);
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
}

void Pty::logout()
{
}

void Pty::readMaster()
{
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "readMaster" << data << len;
}

void Pty::writeMaster()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_master, bytes.constData(), bytes.size());
    //qDebug() << "writeMaster";
}

void Pty::readSlave()
{
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "readSlave" << data << len;
}

void Pty::writeSlave()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_slave, bytes.constData(), bytes.size());
    //qDebug() << "writeSlave";
}

void Pty::setupChildProcess()
{
    dup2(m_slave, STDIN_FILENO);
    dup2(m_slave, STDOUT_FILENO);
    dup2(m_slave, STDERR_FILENO);
}

FileDescriptor Pty::master() const
{
    return m_master;
}

FileDescriptor Pty::slave() const
{
    return m_slave;
}
