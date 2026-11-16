#include "LocalCopyProvider.hpp"
#include "RootShell.hpp"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QFileInfo>
#include <QSettings>

namespace {

const char *kName = "BB-FME";

const char *kOrg = "bbfme";
const char *kApp = "source";

QString name()    { return QString::fromLatin1(kName); }
QString shared()  { return "/accounts/1000/shared/" + name(); }
QString content() { return "/accounts/1000/shared/misc/" + name(); }
QString record()  { return "/pps/system/filesystem/remote/" + name(); }

}  // namespace

QString LocalCopyProvider::methodId() const    { return QString::fromLatin1("local"); }
QString LocalCopyProvider::methodLabel() const { return QString::fromLatin1("On device"); }
QString LocalCopyProvider::sourceName() const  { return name(); }
QString LocalCopyProvider::mountPath() const   { return content(); }

bool LocalCopyProvider::isRegistered() const
{
    // The shared symlink is in the access_shared area, so the app can lstat it
    // without root; its presence means the source is registered.
    return QFileInfo(shared()).isSymLink() || QFile::exists(shared());
}

QString LocalCopyProvider::savedHost() const
{
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).value("local/host").toString();
}

QString LocalCopyProvider::savedPort() const
{
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).value("local/port").toString();
}

QString LocalCopyProvider::registerSource(const QString &host, int port)
{
    QStringList log;

    // Remember the parameters so the Registered card can be restored on relaunch.
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.setValue("local/host", host);
    s.setValue("local/port", port);

    // 1. Fetch a file from the host's basic HTTP server into the content folder.
    if (!QDir().mkpath(content())) {
        log << QString("content dir FAIL: cannot create %1").arg(content());
    } else {
        QString url = QString("http://%1:%2/").arg(host).arg(port);
        QNetworkAccessManager nam;
        QNetworkReply *rep = nam.get(QNetworkRequest(QUrl(url)));

        // Guard timer: a silently-dropping server must not freeze the UI thread.
        QEventLoop loop;
        QTimer guard;
        guard.setSingleShot(true);
        QObject::connect(rep, SIGNAL(finished()), &loop, SLOT(quit()));
        QObject::connect(&guard, SIGNAL(timeout()), &loop, SLOT(quit()));
        guard.start(8000);
        loop.exec();

        if (!rep->isFinished()) {
            rep->abort();
            log << "fetch FAIL: timed out after 8s (no response from server)";
        } else if (rep->error() == QNetworkReply::NoError) {
            QByteArray body = rep->readAll();
            QFile f(content() + "/from-host.txt");
            if (f.open(QIODevice::WriteOnly)) {
                f.write(body);
                f.close();
                log << QString("fetch OK: %1 bytes -> %2/from-host.txt").arg(body.size()).arg(content());
            } else {
                log << "fetch OK but local write FAILED";
            }
        } else {
            log << QString("fetch FAIL: %1").arg(rep->errorString());
        }
        rep->deleteLater();
    }

    // 2. Publish our own source as root: the shared symlink + the remote record.
    QList<QPair<QString, QString> > rec;
    rec << qMakePair(QString::fromLatin1("name"), name())
        << qMakePair(QString::fromLatin1("display_label"), name() + " (on device)")
        << qMakePair(QString::fromLatin1("mount"), content())
        << qMakePair(QString::fromLatin1("public"), shared())
        << qMakePair(QString::fromLatin1("online"), QString::fromLatin1("true"))
        << qMakePair(QString::fromLatin1("perimeter"), QString::fromLatin1("personal"))
        << qMakePair(QString::fromLatin1("pausable"), QString::fromLatin1("false"))
        << qMakePair(QString::fromLatin1("pinnable"), QString::fromLatin1("false"))
        << qMakePair(QString::fromLatin1("supports_extended_provider_data"),
                     QString::fromLatin1("false"));

    QString body;
    for (int i = 0; i < rec.size(); ++i) {
        body += rec.at(i).first + "::" + rec.at(i).second + "\n";
    }

    QString script =
        QString::fromLatin1(
            "PUB=%1\n"
            "REC=%2\n"
            "rm -f \"$PUB\"; ln -s '%3' \"$PUB\" && echo symlink_OK || echo symlink_FAIL\n"
            "cat > \"$REC\" <<'PPS_EOF' && echo record_OK || echo record_FAIL\n"
            "%4"
            "PPS_EOF\n")
            .arg(shared())
            .arg(record())
            .arg(content())
            .arg(body);

    log << "register via /bin/__root: " + RootShell::run(script).replace("\n", " ");
    return log.join("\n");
}

QString LocalCopyProvider::unregisterSource()
{
    QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).remove("local");
    QString script =
        QString::fromLatin1(
            "rm -f '%1' '%2'\n"
            "[ -e '%2' ] && echo record_present || echo record_removed\n")
            .arg(shared())
            .arg(record());
    return "unregister via /bin/__root: " + RootShell::run(script).replace("\n", " ");
}
