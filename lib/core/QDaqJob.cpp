#include "QDaqJob.h"
#include "QDaqSession.h"

QDaqJob::QDaqJob(const QString& name) :
    QDaqObject(name), armed_(false), program_(0), isLoop_(false)
{
}
QDaqJob::~QDaqJob(void)
{
}
void QDaqJob::attach()
{
    QDaqObject::attach();
}
void QDaqJob::detach()
{
    setArmed(false);
    QDaqObject::detach();
}
bool QDaqJob::throwIfArmed()
{
	if (armed_) throwScriptError("Not possible when armed");
	return armed_;
}
bool QDaqJob::exec()
{
    bool ret = true;
    //** Job has been locked at the QDaqLoop level
	if (armed_)
	{
        // run this job's task
        // and then execute all child tasks
        ret = run() && subjobs_.exec();
	}
    return ret;
}
bool QDaqJob::run()
{
    QString msg;
    if (program_)
    {
        if (!loop_eng_) {
            pushError("Loop script engine not available");
            return false;
        }
        if (!loop_eng_->evaluate(*program_,msg))
        {
            pushError("Error executing script job",msg);
            return false;
        }
    }
    return true;
}
bool QDaqJob::arm_()
{
    //disarm_();

    if (!code_.isEmpty())
    {
        loop_eng_ = loopEngine();
        if (!loop_eng_) {
            throwScriptError("Loop script engine not available for executing job script.");
            return false;
        }

        if (!loop_eng_->canEvaluate(code_))
        {
            throwScriptError("Error in job script code.");
            return false;
        }

        program_ = new QScriptProgram(code_,objectName() + "_code");
    }

    armed_ = true;
    return armed_;
}
void QDaqJob::disarm_()
{
    if (program_)
    {
        delete program_;
        program_ = 0;
    }
    armed_ = false;
}

bool QDaqJob::setArmed(bool on)
{
    if (on == armed_) return armed_;

    if (armed_) // disarm
    {
        jobLock();
        disarm_();
        foreach(QDaqJob* j, subjobs_) j->setArmed(false);
        armed_ = false;
        jobUnlock();
    }
    else // arm
    {
        // select my subjobs
        subjobs_.clear();
        foreach(QDaqObject* obj, children_)
        {
            QDaqJob* job = qobject_cast<QDaqJob*>(obj);
            if (job) subjobs_ << job;
        }

        // lock & arm me and my sub-jobs
        jobLock();
        bool ok = true;
        JobList::iterator i = subjobs_.begin();
        while(ok && i!=subjobs_.end()) {
            // arm only the jobs
            // the loops must get extra arm command
            if (!(*i)->isLoop_) ok = (*i)->setArmed(true);
            ++i;
        }


        // Some job failed to arm.
        // the job that failed must send an error
        if (!ok)
        {

            //disarm_();
            foreach(QDaqJob* j, subjobs_)
                if (!j->isLoop_) j->setArmed(false);
            armed_ = false;
        }
        else arm_();


        jobUnlock();

    }
	emit propertiesChanged();
	return armed_;
}
void QDaqJob::setCode(const QString& s)
{
    if (s==code_) return;

    {
        bool onlineChange = armed_;
        if (onlineChange)
        {
            jobLock();
            disarm_();
        }
        code_ = s;
        if (onlineChange)
        {
            arm_();
            jobUnlock();
        }
        emit propertiesChanged();
    }
}
QDaqLoop* QDaqJob::topLoop() const
{
    QDaqObject* p = (QDaqObject*)this;
    QDaqLoop* q = 0;
    while(p && p!=(QDaqObject*)root())
    {
        QDaqLoop* q1 = qobject_cast<QDaqLoop*>(p);
        if (q1) q = q1;
        p = p->parent();
    }
    return q;
}
QDaqLoop* QDaqJob::loop() const
{
    QDaqObject* p = (QDaqObject*)this;
    while(p && p!=(QDaqObject*)root())
    {
        QDaqLoop* q = qobject_cast<QDaqLoop*>(p);
        if (q) return q;
        p = p->parent();
    }
    return 0;
}
QDaqScriptEngine* QDaqJob::loopEngine() const
{
    QDaqLoop* p = loop();
    QDaqScriptEngine* e = 0;
    while (p)
    {
        e = p->loopEngine();
        if (e) break;
        p = p->parentLoop();
    }
    return e;
}
//////////////////// QDaqLoop //////////////////////////////////////////
QDaqLoop::QDaqLoop(const QString& name) :
    QDaqJob(name), count_(0), limit_(0), delay_(0), preload_(0), period_(1000)
{
    isLoop_ = true;
    connect(this,SIGNAL(abort()),this,SLOT(disarm()),Qt::QueuedConnection);
}
QDaqLoop::~QDaqLoop(void)
{
}
bool QDaqLoop::exec()
{
    if (!armed_) return true;

    if (aborted_) return false;

    // check time for loop statistics
    t_[1] = (float)clock_.sec();

    bool ret = true;
    comm_lock.lock();
    if (delay_counter_) delay_counter_--;
    if (delay_counter_ == 0) // loop executes
    {
        // Lock  subjobs
        subjobs_.lock();
        // call base-class exec
        ret = QDaqJob::exec();
        // reset counter
        delay_counter_ = delay_;
        // increase count
        count_++;
        // unlock everything in reverse order
        subjobs_.unlock();

        emit propertiesChanged();
        emit updateWidgets();
    }
    comm_lock.unlock();

    if (ret && limit_ && count_>=limit_)  ret = false;

    if (!ret) {
        aborted_ = true;
        emit abort();
    }

    // loop statistics
    perfmon[0] << (t_[1] - t_[0])*1000; t_[0] = t_[1];
    perfmon[1] << ((float)clock_.sec() - t_[1])*1000;

    return ret;
}

bool QDaqLoop::arm_()
{
    count_ = 0;
    delay_counter_ = preload_;
    aborted_ = false;
    bool ret = QDaqJob::arm_();
    if (ret)
    {
        clock_.start();
        t_[0] = (float)clock_.sec();
        if (isTop()) armed_ = thread_.start(this,period_); //,THREAD_PRIORITY_TIME_CRITICAL);
    }
    return armed_;
}

void QDaqLoop::disarm_()
{
    thread_.stop();
    QDaqJob::disarm_();
}

void QDaqLoop::setLimit(uint d)
{
    if (limit_ != d)
    {
        // locked code
        {
            os::auto_lock L(comm_lock);
            limit_ = d;
        }
        emit propertiesChanged();
    }
}

void QDaqLoop::setDelay(uint d)
{
    if (delay_ != d)
    {
        // locked code
        {
            os::auto_lock L(comm_lock);
            delay_ = d;
            //counter_ = delay_;
        }
        emit propertiesChanged();
    }
}

void QDaqLoop::setPreload(uint d)
{
    if (preload_ != d)
    {
        preload_ = d;
        emit propertiesChanged();
    }
}

void QDaqLoop::setPeriod(unsigned int p)
{
    if (p<10) p=10; // minimum 10 ms
    if (period_ != p)
    {
        bool onlineChange = armed_;
        if (onlineChange)
        {
            jobLock();
            disarm_();
        }
        period_ = p;
        if (onlineChange)
        {
            arm_();
            jobUnlock();
        }
        emit propertiesChanged();
    }
}

QString QDaqLoop::stat()
{
    QString S("Loop statistics:");
    S += QString("\n  Period (ms): %1").arg(perfmon[0]());
    S += QString("\n  Load-time (ms): %2").arg(perfmon[1]());
    return S;
}

void QDaqLoop::createLoopEngine()
{
    if (throwIfArmed()) return;

    if (loop_eng_)
    {
        if (loop_eng_->parent() == (QObject*)this)
        {
            QDaqScriptEngine* e = loop_eng_;
            loop_eng_.clear();
            delete e;
        }
        else loop_eng_.clear();
    }

    loop_eng_ = new QDaqScriptEngine(this);
}
QDaqScriptEngine* QDaqLoop::loopEngine() const
{
    return loop_eng_;
}
QDaqLoop* QDaqLoop::parentLoop() const
{
    QDaqObject* p = parent();
    while(p && p!=(QDaqObject*)root())
    {
        QDaqLoop* q = qobject_cast<QDaqLoop*>(p);
        if (q) return q;
        p = p->parent();
    }
    return 0;
}









