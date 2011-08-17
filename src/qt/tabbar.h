#ifndef __GUI_QT_TABBAR__
#define __GUI_QT_TABBAR__

#include <Qt/QtGui>

class TabBar: public QTabBar
{
	Q_OBJECT
public:
	TabBar(QWidget *parent=0);

protected:
	virtual void mouseReleaseEvent(QMouseEvent *);

};

#endif
