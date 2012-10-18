#ifndef VIM_QT_VIMEVENT
#define VIM_QT_VIMEVENT

#include <QPoint>
#include <QList>
#include <QUrl>

struct VimWrapper;
// Base class
class VimEvent {
public:
	VimEvent(VimWrapper& vim);
	virtual void handle()=0;
protected:
	VimWrapper& vim;
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
