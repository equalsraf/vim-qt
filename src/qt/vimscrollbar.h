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
	int index() const;
	int length();

	bool operator<(const VimScrollBar& other) { return ( m_index < other.m_index ); }

	void setVisible(bool);
	void setLength(int);
signals:
	void indexChanged(int);
	void visibilityChanged(bool);

protected slots:
	void dragging();
	void draggingFinished();

private:
	scrollbar_T *sb;
	int m_index;
	int m_length;
};

#endif
