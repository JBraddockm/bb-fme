/*
 * WebDavProvider — approach B, "WebDAV".
 *
 * Hands a remote WebDAV server to the device's shipped sys.cfs.webdav.todtm
 * resmgr, which mounts it live (no local copy). Registration, as root:
 *   1. merge an entry {server_name, server_addr, username, password, port} into
 *      the servers array of /pps/system/cfs/webdav/status — read/modify/write so
 *      other servers (e.g. BlackBerry Link's own) are preserved, never clobbered;
 *   2. publish the File Manager source ourselves — a /pps/system/filesystem/remote
 *      record + /accounts/1000/shared symlink pointing at the resmgr's sandbox
 *      mount — so the source appears and is remembered at once, instead of only
 *      after the resmgr cold-launches;
 *   3. poke /pps/services/webdav/control to wake/launch the resmgr. When the File
 *      Manager opens the source, cfsd launches the resmgr, which reads the status
 *      entry, curls the server (Basic/Digest) and takes over the mount live.
 * No mDNS/bp2p advert is needed for a reachable server (verified on device).
 */
#ifndef WEBDAVPROVIDER_HPP_
#define WEBDAVPROVIDER_HPP_

#include "SourceProvider.hpp"

#include <QString>

class WebDavProvider : public SourceProvider
{
public:
    QString methodId() const;
    QString methodLabel() const;
    QString sourceName() const;

    /*! The http://host:port URL shown in the UI for the mounted server. */
    QString serverUrl(const QString &host, int port) const;

    /*! True if the source currently exists (checked on app launch). */
    bool isRegistered() const;

    /*! Last-used register parameters, persisted so the card restores on relaunch. */
    QString savedHost() const;
    QString savedPort() const;
    QString savedUser() const;

    /*! Register/update the WebDAV server (host, port, HTTP Basic credentials)
     *  by merging it into the status PPS and poking the resmgr. */
    QString registerSource(const QString &host, int port,
                           const QString &user, const QString &pass);

    QString unregisterSource();
};

#endif /* WEBDAVPROVIDER_HPP_ */
