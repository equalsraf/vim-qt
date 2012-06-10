#ifndef __GUI_QT_VIMACTION__
#define __GUI_QT_VIMACTION__

#include "qvimshell.h"

class VimAction: public QAction
{
	Q_OBJECT
public:
	VimAction(vimmenu_T *, QObject* );

protected slots:
	void actionTriggered();

private:
	vimmenu_T *m_menu;
	static QStringList iconNames;
};

#endif
