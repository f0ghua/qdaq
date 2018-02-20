#include "QDaqDevice.h"

QDaqDevice::QDaqDevice(const QString& name) :
    QDaqJob(name),
    addr_(0),
    online_(false),
    eot_(0x02 << 8),
    eos_(0x100),
    buff_(4096,char(0)),
    buff_sz_(4096)
{
}
QDaqDevice::~QDaqDevice()
{
}
void QDaqDevice::detach()
{
    QDaqJob::detach();
	setOnline_(false);
}
// on-off
void QDaqDevice::setOnline(bool on)
{
	if (throwIfArmed()) return;
    os::auto_lock L(comm_lock);
	if (on!=online_)
	{
		setOnline_(on);
		emit propertiesChanged();
	}
}
bool QDaqDevice::on()
{
	setOnline(true);
	return online_; 
}
void QDaqDevice::off()
{
	setOnline(false);
} 
bool QDaqDevice::setOnline_(bool on)
{
	if (on==online_) return online_;
    if (on && ifc_)
	{
		// set this device to on
        online_ = ifc_->open_port(addr_,this);
        if (online_) ifc_->clear_port(addr_);
		// switch-on also the sub-devices
		if (online_)
		{
			bool ok = true;
			int k = 0;
            foreach(QDaqJob* j, subjobs_)
			{
                QDaqDevice* dev = qobject_cast<QDaqDevice*>(j);
				if (dev)
				{
					ok = dev->setOnline_(true);
					if (!ok) break;
					k++;
				}
			}
			if (!ok) // switch off all
			{
				online_ = false;
                ifc_->close_port(addr_);
                foreach(QDaqJob* j, subjobs_)
				{
                    QDaqDevice* dev = qobject_cast<QDaqDevice*>(j);
					if (dev)
					{
						ok = dev->setOnline_(false);
						k--;
						if (k==0) break;
					}
				}
			}
		}
		return online_;
	}
	else {  
        if (ifc_) ifc_->close_port(addr_); // ifc should excist
		return online_ = false;
	}
}
void QDaqDevice::forcedOffline(const QString& reason)
{
    os::auto_lock L(comm_lock);
	if (armed_) disarm_();// forcedDisarm(reason);
	if (online_) 
	{
		setOnline_(false);
		pushError("forced offline", reason);
	}
}
// setters
void QDaqDevice::setBufferSize(unsigned int sz)
{
	if (throwIfArmed()) return;
    os::auto_lock L(comm_lock);
    buff_.resize(sz);
    buff_sz_ = sz;
	emit propertiesChanged();
}
void QDaqDevice::setAddress(int a)
{
	if (throwIfOnline()) return;
    os::auto_lock L(comm_lock);
	addr_ = a;
	emit propertiesChanged();
}
void QDaqDevice::setEot(int e)
{
	if (throwIfArmed()) return;
    os::auto_lock L(comm_lock);
	eot_ = e;
}
void QDaqDevice::setEos(int e)
{
	if (throwIfArmed()) return;
    os::auto_lock L(comm_lock);
	eos_ = e;
}
void QDaqDevice::setInterface(QDaqObject* o)
{
    if (o==ifc_) return;

    QDaqInterface* i = 0;
    if (o) {
        i = qobject_cast<QDaqInterface*>(o);
        if (!i) {
            throwScriptError(QString("Object %1 is not a QDaqInterface").arg(o->path()));
            return;
        }
    }

    if (throwIfArmed()) return;
    if (throwIfOnline()) return;

    ifc_ = i;

	emit propertiesChanged();
}
// states / messages
bool QDaqDevice::throwIfOffline()
{
	bool ret = !online_;
	if (ret) throwScriptError("Not possible when device is offline.");
	return ret;
}
bool QDaqDevice::throwIfOnline()
{
	if (online_) throwScriptError("Not possible when device is online.");
	return online_;
}
// arming
bool QDaqDevice::arm_()
{
	if (throwIfOffline()) return armed_ = false;
    else return QDaqJob::arm_();
}
// io
int QDaqDevice::write_(const char* msg, int len)
{
    os::auto_lock L(comm_lock);
    int ret = ifc_->write(addr_, msg, len, eot_);
	//if (!ret && armed_) forcedDisarm(QString("Write to device %1 failed").arg(path()));
	checkError(msg,len);
	return ret;
}
int QDaqDevice::write_(const char* msg)
{
	return write_(msg, strlen(msg));
}
int QDaqDevice::write_(const QByteArray& msg)
{
	return write_(msg.constData(),msg.size());
}
bool QDaqDevice::write_(const QList<QByteArray>& msglist)
{
    os::auto_lock L(comm_lock);
	foreach(const QByteArray& msg, msglist)
	{
		if (msg.size() != write_(msg)) return false;
	}
	return true;
}
int QDaqDevice::write(const QString& msg)
{
	//if (throwIfArmed()) return 0;
	if (throwIfOffline()) return 0;
	return write_(msg.toLatin1());
}
int QDaqDevice::write(const QByteArray& msg)
{
    //if (throwIfArmed()) return 0;
    if (throwIfOffline()) return 0;
    return write_(msg);
}
int QDaqDevice::write(int reg, int val) // write
{
    if (throwIfOffline()) return 0;
    os::auto_lock L(comm_lock);
    unsigned short b = val;
    int ret = ifc_->write(reg, (const char *)(&b), sizeof(b), eot_);
    return ret;
}
int QDaqDevice::write(int start_reg, const QByteArray& msg) // write
{
    if (throwIfOffline()) return 0;
    os::auto_lock L(comm_lock);
    int ret = ifc_->write(start_reg, msg.constData(), msg.length(), eot_);
    return ret;
}

QByteArray QDaqDevice::read_()
{
    os::auto_lock L(comm_lock);
    buff_.resize(buff_sz_);
    char* mem = buff_.data();
    int cnt =  ifc_->read(addr_,mem,buff_sz_,eos_);
    buff_.resize(cnt);
	//if (!buff_cnt_ && armed_) forcedDisarm(QString("Read from device %1 failed").arg(path())); 
    return buff_;
}
QByteArray QDaqDevice::read()
{
    if (throwIfOffline()) return QByteArray();
    return read_();
}
// read register from device (modbus type)
int QDaqDevice::read(int reg)
{
    if (throwIfOffline()) return 0;

    os::auto_lock L(comm_lock);

    unsigned short b = 0;
    ifc_->read(reg,(char *)(&b),sizeof(b),eos_);

    return b;
}
// read n consequtive registers from device (modbus type)
QByteArray QDaqDevice::read(int reg, int n)
{
    if (throwIfOffline()) return QByteArray();
    os::auto_lock L(comm_lock);
    buff_.resize(2*n);
    char* mem = buff_.data();
    int cnt = ifc_->read(reg,mem,2*n,eos_);
    buff_.resize(cnt);
    return buff_;
}

QString QDaqDevice::query(const QString& msg)
{
	//if (throwIfArmed()) return QString();
	if (throwIfOffline()) return QString();
    os::auto_lock L(comm_lock);
	write_(msg.toLatin1());
	return QString(read_());
}
int QDaqDevice::statusByte_()
{
    return ifc_->readStatusByte(addr_);
}
int QDaqDevice::statusByte()
{
	if (throwIfArmed()) return 0;
	if (throwIfOffline()) return 0;
	return statusByte_();
}
void QDaqDevice::clear()
{
	if (throwIfArmed()) return;
	if (throwIfOffline()) return;
    ifc_->clear_port(addr_);
}
