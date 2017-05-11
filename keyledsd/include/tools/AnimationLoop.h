#ifndef KEYLEDSD_ANIM_LOOP_H
#define KEYLEDSD_ANIM_LOOP_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

struct timespec;

namespace keyleds {

class AnimationLoop : public QThread
{
    Q_OBJECT
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged);

public:
                    AnimationLoop(unsigned fps, QObject * parent = 0);
                    ~AnimationLoop();
    bool            isPaused() const { return paused; }
    int             getError() const { return error; }

public slots:
    void            setPaused(bool paused = true);

signals:
    void            pausedChanged(bool paused);
    void            rendered(void);

protected:
    void            scheduleNextTick(struct timespec * res);
    virtual bool    render();

    void            run() override;

private:
    QMutex          mRunStatus;
    QWaitCondition  cRunStatus;

    unsigned        fps;
    bool            paused;
    bool            abort;
    int             error;
};

};

#endif
