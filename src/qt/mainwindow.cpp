#include "mainwindow.moc"

#include "../runtime/vim32x32.xpm"

MainWindow::MainWindow( gui_T* gui, QWidget *parent)
:QMainWindow(parent)
{
	setWindowIcon(QPixmap(vim32x32));

	// Menu
	menutoolbar = addToolBar("Menu");
	menutoolbar->setObjectName("menu");
	menu = new QMenuBar(menutoolbar);
	menutoolbar->addWidget(menu);

	// Tool bar
	toolbar = addToolBar("ToolBar");
	toolbar->setObjectName("toolbar");

	// Vim shell
	vimshell = new QVimShell( gui, this );
	setCentralWidget(vimshell);
	vimshell->setFocus();

	// TabLine
	tabtoolbar = addToolBar("tabline");
	tabtoolbar->setObjectName("tabline");

	tabbar = new QTabBar(tabtoolbar);
	tabbar->setTabsClosable(true);
	tabbar->setExpanding(false);
	tabbar->setFocusPolicy(Qt::NoFocus);


	tabtoolbar->addWidget(tabbar);

	connect( tabbar, SIGNAL(tabCloseRequested(int)),
			this, SLOT(closeTab(int)));
	connect( tabbar, SIGNAL(currentChanged(int)),
			this, SLOT(switchTab(int)));

}

QVimShell* MainWindow::vimShell()
{
	return this->vimshell;
}

QMenuBar* MainWindow::menuBar() const
{
	return menu;
}

QToolBar* MainWindow::toolBar() const
{
	return toolbar;
}

void MainWindow::closeEvent (QCloseEvent * event)
{
	vimshell->closeEvent(event);
}

void MainWindow::showTabline(bool show)
{
	tabtoolbar->setVisible(show);
}

void MainWindow::showToolbar(bool show)
{
	toolbar->setVisible(show);
}

void MainWindow::showMenu(bool show)
{
	menutoolbar->setVisible(show);
}

bool MainWindow::tablineVisible()
{
	return tabtoolbar->isVisible();
}

void MainWindow::setCurrentTab(int idx)
{
	tabbar->setCurrentIndex(idx);
}


void MainWindow::setTab( int idx, const QString& label)
{
	while ( tabbar->count() <= idx ) {
		tabbar->addTab("[No name]");
	}

	tabbar->setTabText(idx, label);
}


void MainWindow::removeTabs(int idx)
{
	int i;
	for ( i=idx; i<tabbar->count(); i++) {
		tabbar->removeTab(i);
	}
}

void MainWindow::switchTab(int idx)
{
	vimshell->switchTab(idx+1);
}

void MainWindow::closeTab(int idx)
{
	vimshell->closeTab(idx+1);
}
