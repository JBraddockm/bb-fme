/*
 * LocalCopyProvider — approach A, "Local copy".
 *
 * Fetches a file from an HTTP server on the host into a folder on the device,
 * then publishes a File Manager source backed by that local folder: a
 * /accounts/1000/shared/<name> symlink + a /pps/system/filesystem/remote/<name>
 * record (both written as root). The files are stored on the device.
 */
#ifndef LOCALCOPYPROVIDER_HPP_
#define LOCALCOPYPROVIDER_HPP_

#include "SourceProvider.hpp"

#include <QString>

class LocalCopyProvider : public SourceProvider
{
public:
    QString methodId() const;
    QString methodLabel() const;
    QString sourceName() const;

    /*! The on-device folder the source serves (and where the fetch lands). */
    QString mountPath() const;

    /*! True if the source currently exists (checked on app launch). */
    bool isRegistered() const;

    /*! Last-used register parameters, persisted so the card restores on relaunch. */
    QString savedHost() const;
    QString savedPort() const;

    /*! Fetch http://host:port/ into the content folder, then register the
     *  source as root. Returns a multi-line diagnostic. */
    QString registerSource(const QString &host, int port);

    QString unregisterSource();
};

#endif /* LOCALCOPYPROVIDER_HPP_ */
