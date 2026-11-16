#include "WebDavProvider.hpp"
#include "RootShell.hpp"

#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QFileInfo>
#include <QFile>
#include <QSettings>

#include <bb/data/JsonDataAccess>

using namespace bb::data;

namespace {

const char *kName    = "BB-FME-DAV";
const char *kStatus  = "/pps/system/cfs/webdav/status";
const char *kControl = "/pps/services/webdav/control";
const char *kOrg     = "bbfme";
const char *kApp     = "source";

QString name()   { return QString::fromLatin1(kName); }
QString shared() { return "/accounts/1000/shared/" + name(); }
QString record() { return "/pps/system/filesystem/remote/" + name(); }

/*! Read the current servers array from the status PPS (as root, via cat). */
QVariantList readServers()
{
    QString text = RootShell::run(QString::fromLatin1("cat %1").arg(QString::fromLatin1(kStatus)));
    const QString key = QString::fromLatin1("servers:json:");
    int i = text.indexOf(key);
    if (i < 0) {
        return QVariantList();
    }
    int start = i + key.length();
    int end = text.indexOf(QChar('\n'), start);
    QString json = (end < 0) ? text.mid(start) : text.mid(start, end - start);

    JsonDataAccess jda;
    QVariant v = jda.loadFromBuffer(json.trimmed().toUtf8());
    return jda.hasError() ? QVariantList() : v.toList();
}

QString jsonString(const QString &s)
{
    QString e = s;
    e.replace(QChar('\\'), QString::fromLatin1("\\\\"));
    e.replace(QChar('"'), QString::fromLatin1("\\\""));
    return QChar('"') + e + QChar('"');
}

/*! Serialize one server entry compactly (no newlines — it is one PPS attr). */
QString serverJson(const QVariantMap &m)
{
    return QString::fromLatin1(
               "{\"server_name\":%1,\"server_addr\":%2,\"username\":%3,"
               "\"password\":%4,\"port\":%5}")
        .arg(jsonString(m.value("server_name").toString()))
        .arg(jsonString(m.value("server_addr").toString()))
        .arg(jsonString(m.value("username").toString()))
        .arg(jsonString(m.value("password").toString()))
        .arg(m.value("port").toInt());
}

/*! Write the whole servers array back to the status PPS (as root). */
QString writeServers(const QVariantList &servers)
{
    QStringList items;
    for (int i = 0; i < servers.size(); ++i) {
        items << serverJson(servers.at(i).toMap());
    }
    QString json = QString::fromLatin1("[") + items.join(QString::fromLatin1(",")) + QString::fromLatin1("]");
    // JSON uses only double quotes, so single-quote-wrapping in the shell is safe.
    QString script = QString::fromLatin1("echo 'servers:json:%1' >> %2\n")
                         .arg(json)
                         .arg(QString::fromLatin1(kStatus));
    return RootShell::run(script);
}

/*! The servers list with any existing entry for @p keepOutName removed. */
QVariantList without(const QVariantList &servers, const QString &keepOutName)
{
    QVariantList kept;
    for (int i = 0; i < servers.size(); ++i) {
        if (servers.at(i).toMap().value("server_name").toString() != keepOutName) {
            kept << servers.at(i);
        }
    }
    return kept;
}

}  // namespace

QString WebDavProvider::methodId() const    { return QString::fromLatin1("webdav"); }
QString WebDavProvider::methodLabel() const { return QString::fromLatin1("WebDAV"); }
QString WebDavProvider::sourceName() const  { return name(); }

QString WebDavProvider::serverUrl(const QString &host, int port) const
{
    return QString::fromLatin1("http://%1:%2").arg(host).arg(port);
}

bool WebDavProvider::isRegistered() const
{
    // Approach B is remembered by our own persisted registration, not by the
    // resmgr's shared symlink. That symlink appears only once the shipped
    // webdav resmgr has launched and mounted the server, which can lag behind
    // registration (the resmgr cold-launches later, e.g. when the File Manager
    // opens). The QSettings entry is written the moment we register, so the
    // Registered card is restored on relaunch whether or not the live mount is
    // up yet — the user can always see, update or delete the source.
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp))
        .contains("webdav/host");
}

QString WebDavProvider::savedHost() const
{
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).value("webdav/host").toString();
}

QString WebDavProvider::savedPort() const
{
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).value("webdav/port").toString();
}

QString WebDavProvider::savedUser() const
{
    return QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).value("webdav/user").toString();
}

QString WebDavProvider::registerSource(const QString &host, int port,
                                       const QString &user, const QString &pass)
{
    QStringList log;

    // Remember the parameters so the Registered card can be restored on relaunch.
    {
        QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
        s.setValue("webdav/host", host);
        s.setValue("webdav/port", port);
        s.setValue("webdav/user", user);
    }

    // 1. Merge our entry into the existing servers list (preserve the others).
    QVariantList servers = without(readServers(), name());
    QVariantMap me;
    me["server_name"] = name();
    me["server_addr"] = QString::fromLatin1("http://") + host;  // host-only; port is separate
    me["username"] = user;
    me["password"] = pass;
    me["port"] = port;
    servers << me;
    log << "status merge: " + writeServers(servers).replace("\n", " ");

    // 2. Publish the File Manager source ourselves (record + shared symlink), so
    //    it appears and is remembered immediately — without waiting for the
    //    shipped resmgr to cold-launch. The resmgr owns the live mount under its
    //    own sandbox (fs/mounts/<name>), whose path we resolve at runtime (the
    //    app's install id is not fixed across devices). When the File Manager
    //    opens the source, cfsd launches the resmgr, which reads the status entry
    //    and mounts the server live, taking over this record.
    log << "publish source: "
            + RootShell::run(
                  QString::fromLatin1(
                      "N=%1\n"
                      "for d in /accounts/1000/appserv/sys.cfs.webdav.todtm.*; do"
                      " [ -d \"$d\" ] && APPSERV=\"$d\"; done\n"
                      "if [ -n \"$APPSERV\" ] && [ -d \"$APPSERV\" ]; then\n"
                      "  APPID=${APPSERV##*/}\n"
                      "  MNT=\"$APPSERV/fs/mounts/$N\"\n"
                      "  PUB=\"/accounts/1000/shared/$N\"\n"
                      "  REC=\"/pps/system/filesystem/remote/$N\"\n"
                      "  rm -f \"$PUB\"; ln -s \"$MNT\" \"$PUB\""
                      " && echo symlink_OK || echo symlink_FAIL\n"
                      "  cat > \"$REC\" <<PPS_EOF && echo record_OK || echo record_FAIL\n"
                      "name::$N\n"
                      "app_name::$APPID\n"
                      "_SERVICEPATH::$APPSERV/fs/mounts\n"
                      "mount::$MNT\n"
                      "public::$PUB\n"
                      "online::true\n"
                      "perimeter::personal\n"
                      "pausable::false\n"
                      "pinnable::false\n"
                      "supports_extended_provider_data::false\n"
                      "PPS_EOF\n"
                      "else echo appserv_not_found; fi\n")
                      .arg(name()))
                  .replace("\n", " ");

    // 3. Poke the control object; this wakes/launches the resmgr, which reads the
    //    status entry and mounts the server (curl, Basic/Digest) — no local copy.
    QString poke =
        QString::fromLatin1(
            "echo 'name::%1' >> %2\n"
            "echo 'url::http://%3' >> %2\n"
            "echo 'username::%4' >> %2\n"
            "echo 'password::%5' >> %2\n"
            "echo 'port:n:%6' >> %2\n")
            .arg(name())
            .arg(QString::fromLatin1(kControl))
            .arg(host)
            .arg(user)
            .arg(pass)
            .arg(port);
    log << "control poke: " + RootShell::run(poke).replace("\n", " ");

    return log.join("\n");
}

QString WebDavProvider::unregisterSource()
{
    QStringList log;
    QSettings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp)).remove("webdav");

    // Drop our entry from the servers list (keep the others), then remove the
    // source record + symlink the resmgr created.
    log << "status merge: " + writeServers(without(readServers(), name())).replace("\n", " ");

    QString rm =
        QString::fromLatin1(
            "rm -f '%1' '%2'\n"
            "[ -e '%2' ] && echo record_present || echo record_removed\n")
            .arg(shared())
            .arg(record());
    log << "remove source: " + RootShell::run(rm).replace("\n", " ");

    return log.join("\n");
}
