#ifndef TOOLS_SESSION_WATCHER
#define TOOLS_SESSION_WATCHER

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <memory>

class SessionWatcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sessionPath READ sessionPath);
    Q_PROPERTY(QString seatPath READ seatPath);
public:
                    SessionWatcher(QObject * parent = nullptr);
    const QString & sessionPath() const { return m_sessionPath; }
    const QString & seatPath() const { return m_seatPath; }

signals:
    void            lockChanged(bool val);
    void            activeChanged(bool val);
    void            idleChanged(bool val);

private slots:
    void            sessionPropertiesChanged(const QString & interface,
                                             const QMap<QString, QVariant> & changed);
    void            sessionLocked();
    void            sessionUnlocked();

protected:
    QDBusInterface  m_loginManager;
    QString         m_sessionPath;
    QString         m_seatPath;
};

#endif
