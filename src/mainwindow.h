#ifndef __GUI_QT_MAINWINDOW__
#define __GUI_QT_MAINWINDOW__

#include <Qt/QtGui>
#include "qvimshell.h"

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(gui_T *, QWidget *parent=0);
	QVimShell* vimShell();

	bool tablineVisible();

	QMenuBar* menuBar() const;

public slots:
	void showTabline(bool show);
	void setCurrentTab(int idx);
	void setTab( int, const QString& );
	void removeTabs(int);
	void switchTab(int idx);
	void closeTab(int idx);

protected:
	virtual void closeEvent( QCloseEvent *);

private:
	QVimShell *vimshell;
	QToolBar *tabtoolbar;
	QTabBar *tabbar;

	QMenuBar *menu;
};

#endif
