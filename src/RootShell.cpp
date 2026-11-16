#include "RootShell.hpp"

#include <QProcess>

QString RootShell::run(const QString &script)
{
    QProcess proc;
    proc.start("/bin/__root");
    if (!proc.waitForStarted(5000)) {
        return QString("/bin/__root did not start (%1)").arg(proc.errorString());
    }
    proc.write(script.toUtf8());
    proc.closeWriteChannel();
    proc.waitForFinished(15000);

    QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString err = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    if (!err.isEmpty()) {
        out += (out.isEmpty() ? QString() : QString(" | ")) + "stderr: " + err;
    }
    return out.isEmpty() ? QString("(no output)") : out;
}
