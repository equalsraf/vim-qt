#ifndef __GUI_QT_VIMSCROLLBAR__
#define __GUI_QT_VIMSCROLLBAR__

#include "qvimshell.h"

#include <QtGui/QScrollBar>

class VimScrollBar: public QScrollBar
{
	Q_OBJECT
public:
	VimScrollBar(scrollbar_T *, Qt::Orientation, QWidget *parent=0);
	void setIndex(int);
	int index();

	bool operator<(const VimScrollBar& other) { return ( m_index < other.m_index ); }

	void setVisible(bool);
signals:
	void indexChanged(int);
	void visibilityChanged(bool);

protected slots:
	void dragging();
	void draggingFinished();

private:
	scrollbar_T *sb;
	int m_index;
};

#endif
