#ifndef VIM_QT_VIMEVENT
#define VIM_QT_VIMEVENT

#include <QPoint>
#include <QList>
#include <QUrl>

struct VimWrapper;
// Base class
class VimEvent {
public:
	enum EvType {Resize, Close, Drop};

	VimEvent(VimWrapper& vim, EvType t);
	virtual void handle()=0;
	EvType type() { return m_type; }
protected:
	VimWrapper& vim;
	EvType m_type;
};


class ResizeEvent: public VimEvent
{
public:
	ResizeEvent(VimWrapper& vim, int w, int h);
	void handle();
private:
	int width, height;

};

class CloseEvent: public VimEvent
{
public:
	CloseEvent(VimWrapper& vim);
	void handle();
};

class DropEvent: public VimEvent
{
public:
	DropEvent(VimWrapper& vim, const QPoint& pos, unsigned int mod, QList<QUrl> urls);
	void handle();
private:
	QPoint pos;
	unsigned int mod;
	QList<QUrl> urls;
};

#endif
