#include "applicationui.hpp"
#include "LocalCopyProvider.hpp"
#include "WebDavProvider.hpp"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/cascades/LocaleHandler>

#include <QLocale>
#include <QTranslator>

using namespace bb::cascades;

ApplicationUI::ApplicationUI() :
        QObject(),
        m_local(new LocalCopyProvider()),
        m_webdav(new WebDavProvider())
{
    m_pTranslator = new QTranslator(this);
    m_pLocaleHandler = new LocaleHandler(this);

    QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()), this, SLOT(onSystemLanguageChanged()));
    onSystemLanguageChanged();

    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
    // Expose this facade to QML; the UI calls the register/unregister methods.
    qml->setContextProperty("_app", this);

    AbstractPane *root = qml->createRootObject<AbstractPane>();
    Application::instance()->setScene(root);
}

ApplicationUI::~ApplicationUI()
{
    delete m_local;
    delete m_webdav;
}

QString ApplicationUI::registerLocal(const QString &host, int port)
{
    return m_local->registerSource(host, port);
}

QString ApplicationUI::unregisterLocal()
{
    return m_local->unregisterSource();
}

QString ApplicationUI::localMountPath() const
{
    return m_local->mountPath();
}

bool ApplicationUI::localIsRegistered() const { return m_local->isRegistered(); }
QString ApplicationUI::localSavedHost() const { return m_local->savedHost(); }
QString ApplicationUI::localSavedPort() const { return m_local->savedPort(); }

QString ApplicationUI::registerWebdav(const QString &host, int port,
                                      const QString &user, const QString &pass, bool secure)
{
    return m_webdav->registerSource(host, port, user, pass, secure);
}

QString ApplicationUI::unregisterWebdav()
{
    return m_webdav->unregisterSource();
}

bool ApplicationUI::webdavIsRegistered() const { return m_webdav->isRegistered(); }
QString ApplicationUI::webdavSavedHost() const { return m_webdav->savedHost(); }
QString ApplicationUI::webdavSavedPort() const { return m_webdav->savedPort(); }
QString ApplicationUI::webdavSavedUser() const { return m_webdav->savedUser(); }
bool ApplicationUI::webdavSavedSecure() const { return m_webdav->savedSecure(); }

void ApplicationUI::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(m_pTranslator);
    QString locale_string = QLocale().name();
    QString file_name = QString("BB-FME_%1").arg(locale_string);
    if (m_pTranslator->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(m_pTranslator);
    }
}
