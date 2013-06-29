#ifndef __GUI_QT_VIMQTMENU__
#define __GUI_QT_VIMQTMENU__

#include <QMenu>
#include "qvimshell.h"

class VimQtMenu: public QMenu
{
	Q_OBJECT
public:
	VimQtMenu(vimmenu_T *, QWidget *);
public slots:
	void setEnableMnemonic(bool enabled);
private:
	QString name, dname;
	bool enableMnemonic;
};

#endif
