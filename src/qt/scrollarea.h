#ifndef __GUI_QT_SCROLLAREA__
#define __GUI_QT_SCROLLAREA__

#include <Qt/QtGui>
#include "vimscrollbar.h"

class ScrollArea: public QWidget
{
	Q_OBJECT
public:
	ScrollArea(QWidget *parent=0);
	void setWidget(QWidget *);

	void addScrollbarRight(VimScrollBar *);
	void addScrollbarBottom(VimScrollBar *);
	void addScrollbarLeft(VimScrollBar *);

protected slots:
	void layoutEast();
	void layoutWest();
	void layoutSouth();

private:
	QWidget *m_widget;
	QGridLayout *m_layout;

	QHBoxLayout *north;
	QHBoxLayout *south;
	QVBoxLayout *east;
	QVBoxLayout *west;
};

#endif
