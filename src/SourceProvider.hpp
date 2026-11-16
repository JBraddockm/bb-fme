/*
 * SourceProvider — the abstract File-Manager-extension strategy.
 *
 * BB10's File Manager can gain a custom source two ways, and this template
 * implements both as sibling strategies:
 *
 *   - LocalCopyProvider ("Local copy") : the app fetches a file from a server
 *     and stores it on the device, then publishes its OWN source — a
 *     /pps/system/filesystem/remote/<name> record + shared symlink. The files
 *     live on the device. (This is what the sys.cfs.dropbox app does.)
 *
 *   - WebDavProvider ("WebDAV") : the app hands a remote WebDAV server to the
 *     device's shipped sys.cfs.webdav.todtm resmgr, which mounts it live. The
 *     files stay on the server and are fetched on demand; nothing is copied.
 *
 * Both register and unregister as root (RootShell); the difference is entirely
 * in what they write and who serves the files, which is why each is its own
 * class behind this interface.
 */
#ifndef SOURCEPROVIDER_HPP_
#define SOURCEPROVIDER_HPP_

#include <QString>

class SourceProvider
{
public:
    virtual ~SourceProvider() {}

    /*! Stable id for the UI / routing: "local" or "webdav". */
    virtual QString methodId() const = 0;

    /*! Human-readable method name shown in the UI. */
    virtual QString methodLabel() const = 0;

    /*! The source name as it appears in the device File Manager. */
    virtual QString sourceName() const = 0;

    /*! Remove the source. Returns a one-line diagnostic for the UI. */
    virtual QString unregisterSource() = 0;
};

#endif /* SOURCEPROVIDER_HPP_ */
