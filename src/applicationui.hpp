/*
 * File Manager source template — QML facade.
 *
 * Owns the two source-provider strategies (LocalCopyProvider, WebDavProvider)
 * and exposes thin, typed invokables to QML. All device work lives in the
 * providers; this class only wires QML to them.
 */
#ifndef ApplicationUI_HPP_
#define ApplicationUI_HPP_

#include <QObject>
#include <QString>

namespace bb
{
    namespace cascades
    {
        class LocaleHandler;
    }
}

class QTranslator;
class LocalCopyProvider;
class WebDavProvider;

class ApplicationUI : public QObject
{
    Q_OBJECT
public:
    ApplicationUI();
    virtual ~ApplicationUI();

    // --- Approach A: Local copy (files fetched and stored on the device) ---
    Q_INVOKABLE QString registerLocal(const QString &host, int port);
    Q_INVOKABLE QString unregisterLocal();
    Q_INVOKABLE QString localMountPath() const;
    Q_INVOKABLE bool localIsRegistered() const;
    Q_INVOKABLE QString localSavedHost() const;
    Q_INVOKABLE QString localSavedPort() const;

    // --- Approach B: WebDAV (remote server mounted live, no local copy) ---
    Q_INVOKABLE QString registerWebdav(const QString &host, int port,
                                       const QString &user, const QString &pass, bool secure);
    Q_INVOKABLE QString unregisterWebdav();
    Q_INVOKABLE bool webdavIsRegistered() const;
    Q_INVOKABLE QString webdavSavedHost() const;
    Q_INVOKABLE QString webdavSavedPort() const;
    Q_INVOKABLE QString webdavSavedUser() const;
    Q_INVOKABLE bool webdavSavedSecure() const;

private slots:
    void onSystemLanguageChanged();

private:
    QTranslator* m_pTranslator;
    bb::cascades::LocaleHandler* m_pLocaleHandler;
    LocalCopyProvider* m_local;
    WebDavProvider* m_webdav;
};

#endif /* ApplicationUI_HPP_ */
