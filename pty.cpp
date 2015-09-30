#include "pty.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <utmpx.h>

#include <QtCore/QDebug>

Pty::Pty()
    : m_master(-1)
    , m_slave(-1)
{
}

Pty::~Pty()
{
}

bool Pty::openPty()
{
    int rc = 0;

    // Open master pseudo terminal
    m_master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (m_master <= 0) {
        qFatal("Could not open a pseudo-terminal device: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    // Get the associated slave name
    m_slaveName = ptsname(m_master);
    if (m_slaveName.isEmpty()) {
        qFatal("Could not get name of the slave pseudo-terminal device: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    // Change the mode and ownership of the slave associated to point to this process
    rc = grantpt(m_master);
    if (rc != 0) {
        qFatal("Could not grant access to the slave pseudo-terminal device: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    // Unlock the slave associated with this master pseudo terminal
    rc = unlockpt(m_master);
    if (rc != 0) {
        qFatal("Could not unlock a pseudo-terminal master/slave pair: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    // Open the slave
    m_slave = ::open(m_slaveName.toAscii().data(), O_RDWR | O_NOCTTY);
    if (m_slave < 0) {
        qFatal("Could not open slave pseudo-terminal: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    rc = fcntl(m_master, F_SETFD, FD_CLOEXEC);
    if (rc == -1) {
        qFatal("Could not set file descriptor flags on master: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    rc = fcntl(m_slave, F_SETFD, FD_CLOEXEC);
    if (rc == -1) {
        qFatal("Could not set file descriptor flags on slave: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    struct ::termios ttmode;
    rc = tcgetattr(m_master, &ttmode);
    if (rc != 0) {
        qFatal("Could not get the parameters associated with the terminal: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    ttmode.c_iflag |= IUTF8;

    rc = tcsetattr(m_master, TCSANOW, &ttmode);
    if (rc != 0) {
        qFatal("Could not set the parameters associated with the terminal: %s (%d).", strerror(errno), errno);
        goto fail;
    }

    return true;

fail:
    closePty();
    return false;
}

void Pty::closePty()
{
    m_slaveName = QString();
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
#if !defined(__APPLE__)
    updwtmpx(_PATH_UTMPX, &loginInfo);
#endif
}

void Pty::logout()
{
    qDebug() << "Pty::logout";
}

void Pty::readMaster()
{
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "Pty::readMaster data:" << data << " len:" << len;
}

void Pty::writeMaster()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_master, bytes.constData(), bytes.size());
    //qDebug() << "Pty::writeMaster";
}

void Pty::readSlave()
{
    char buffer[1025];
    int len = ::read(m_master, buffer, 1024);
    QByteArray data = QByteArray(buffer, len);
    qDebug() << "Pty::readSlave data:" << data << " len:" << len;
}

void Pty::writeSlave()
{
    //QByteArray bytes = "/bin/bash\n";
    //int result = ::write(m_slave, bytes.constData(), bytes.size());
    //qDebug() << "Pty::writeSlave";
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
