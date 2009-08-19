#include "Pty.h"

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
    return false;
}

void Pty::closePty()
{
}

void Pty::login()
{
}

void Pty::logout()
{
}

void Pty::readSlave()
{
}

void Pty::writeSlave()
{
}

void Pty::readMaster()
{
}

void Pty::writeMaster()
{
}

void Pty::setupChildProcess()
{
}

FileDescriptor Pty::master() const
{
    return m_master;
}

FileDescriptor Pty::slave() const
{
    return m_slave;
}
