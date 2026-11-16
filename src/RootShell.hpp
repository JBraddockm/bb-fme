/*
 * RootShell — run a short script as root through /bin/__root.
 *
 * Every File-Manager-source object this app touches is owned by cfs:cfs (the
 * /pps/system/filesystem/remote/<name> records, the /accounts/1000/shared/<name>
 * symlinks, and the /pps/system/cfs/webdav/* objects), so they can only be
 * created or removed as root. On this permissive BB10 build /bin/__root is a
 * setuid-root wrapper that runs its stdin as root; QNX pathtrust lets it exec
 * only trusted system binaries, so the scripts here use cat / ln / rm / echo.
 *
 * Both source providers share this single bridge.
 */
#ifndef ROOTSHELL_HPP_
#define ROOTSHELL_HPP_

#include <QString>

class RootShell
{
public:
    /*! Pipe @p script to /bin/__root and return its combined output (stdout,
     *  with any stderr appended), or a diagnostic if the wrapper did not start. */
    static QString run(const QString &script);
};

#endif /* ROOTSHELL_HPP_ */
