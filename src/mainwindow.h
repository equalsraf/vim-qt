#ifndef __GUI_QT_MAINWINDOW__
#define __GUI_QT_MAINWINDOW__

#include <QtGui>
#include "qvimshell.h"

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(gui_T *, QWidget *parent=0);
	QVimShell* vimShell();

protected:
	virtual void closeEvent( QCloseEvent *);

private:
	QVimShell *vimshell;
};

#endif
