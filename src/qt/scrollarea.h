#ifndef __GUI_QT_SCROLLAREA__
#define __GUI_QT_SCROLLAREA__

#include <QtGui>
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

	static bool scrollBarLessThan(const VimScrollBar *, const VimScrollBar *);

protected slots:
	void layoutEast();
	void layoutWest();
	void layoutSouth();

protected:
	void layoutEdge(QBoxLayout *);

private:
	QWidget *m_widget;
	QGridLayout *m_layout;

	QHBoxLayout *north;
	QHBoxLayout *south;
	QVBoxLayout *east;
	QVBoxLayout *west;
};

#endif
