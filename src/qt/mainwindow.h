#ifndef __GUI_QT_MAINWINDOW__
#define __GUI_QT_MAINWINDOW__

#include <QtGui>
#include "qvimshell.h"
#include "scrollarea.h"
#include "tabbar.h"

class MainWindow: public QMainWindow
{
	Q_OBJECT
	Q_PROPERTY(bool m_keepTabbar READ keepTabbar WRITE setKeepTabbar )
public:
	MainWindow(gui_T *, QWidget *parent=0);
	QVimShell* vimShell();

	bool tablineVisible();

	QMenuBar* menuBar() const;
	QToolBar* toolBar() const;
	bool keepTabbar();

	bool restoreState(const QByteArray& state, int version=0);

	void addScrollbarRight(VimScrollBar *b) { scrollArea->addScrollbarRight(b); }
	void addScrollbarBottom(VimScrollBar *b) { scrollArea->addScrollbarBottom(b); }
	void addScrollbarLeft(VimScrollBar *b) { scrollArea->addScrollbarLeft(b); }

public slots:
	void showTabline(bool show);
	void showMenu(bool show);
	void showToolbar(bool show);
	void setCurrentTab(int idx);
	void setTab( int, const QString& );
	void removeTabs(int);
	void switchTab(int idx);
	void closeTab(int idx);
	void setKeepTabbar(bool);
	void openNewTab();

protected:
	virtual void closeEvent( QCloseEvent *);

protected slots:
	void updateTabOrientation();
	void tabMoved(int from, int to);

private:
	QToolBar *toolbar;

	QVimShell *vimshell;
	QToolBar *tabtoolbar;
	TabBar *tabbar;
	
	QToolBar *menutoolbar;
	QMenuBar *menu;

	bool m_keepTabbar;
	ScrollArea *scrollArea;
};

#endif
