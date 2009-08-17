#ifndef PSEUDOTERMINAL_H
#define PSEUDOTERMINAL_H

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QSocketNotifier>
#include <QtCore/QString>
#include <QtCore/QStringList>

class PseudoTerminal : public QProcess
{
    Q_OBJECT

public:
    PseudoTerminal(QObject *parent);
    ~PseudoTerminal();

    void startPty(const QString &program, const QStringList &arguments);

protected:
    virtual void setupChildProcess();

private Q_SLOTS:
    void encounteredError(QProcess::ProcessError);
    void readStandardError();
    void readStandardOutput();

    void readSlave();
    void writeSlave();
    void readMaster();
    void writeMaster();

private:
    bool openPty();
    void closePty();

    void login();
    void logout();

    void exit();

private:
    QSocketNotifier *m_readMaster;
    QSocketNotifier *m_writeMaster;
    QSocketNotifier *m_readSlave;
    QSocketNotifier *m_writeSlave;
    typedef int FileDescriptor;
    FileDescriptor m_master;
    FileDescriptor m_slave;
    QByteArray m_slaveName;
    bool m_isOpen;
};

#endif // PSEUDOTERMINAL_H
