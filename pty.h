#ifndef PTY_H
#define PTY_H

typedef int FileDescriptor;

class Pty
{
public:
    Pty();
    ~Pty();

    bool openPty();
    void closePty();

    void login();
    void logout();

    void readSlave();
    void writeSlave();
    void readMaster();
    void writeMaster();

    void setupChildProcess();

    FileDescriptor master() const;
    FileDescriptor slave() const;

private:
    FileDescriptor m_master;
    FileDescriptor m_slave;
    const char *m_slaveName;
};

#endif // PTY_H
