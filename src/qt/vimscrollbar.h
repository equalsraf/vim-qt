#ifndef __GUI_QT_VIMSCROLLBAR__
#define __GUI_QT_VIMSCROLLBAR__

#include "qvimshell.h"

#include <QtGui/QScrollBar>

class VimScrollBar: public QScrollBar
{
	Q_OBJECT
public:
	VimScrollBar(scrollbar_T *, Qt::Orientation, QWidget *parent=0);

protected slots:
	void dragging();
	void draggingFinished();

private:
	scrollbar_T *sb;
};

#endif
