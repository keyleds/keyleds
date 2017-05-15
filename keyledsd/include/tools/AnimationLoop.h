#ifndef TOOLS_ANIM_LOOP_H
#define TOOLS_ANIM_LOOP_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

struct timespec;

class AnimationLoop : public QThread
{
    Q_OBJECT
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged);
    Q_PROPERTY(int error READ error);

public:
                    AnimationLoop(unsigned fps, QObject * parent = 0);
                    ~AnimationLoop() override;

    bool            paused() const { return m_paused; }
    int             error() const { return m_error; }

public slots:
    void            setPaused(bool paused);
    void            stop();
signals:
    void            pausedChanged(bool paused);

protected:
    void            scheduleNextTick(struct timespec & next, const struct timespec & prev);
    virtual bool    render(unsigned long) = 0;

    void            run() override;

private:
    QMutex          m_mRunStatus;
    QWaitCondition  m_cRunStatus;

    unsigned        m_period;
    bool            m_paused;
    bool            m_abort;
    int             m_error;
};

#endif
