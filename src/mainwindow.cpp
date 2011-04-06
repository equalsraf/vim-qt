#include "mainwindow.moc"

MainWindow::MainWindow( gui_T* gui, QWidget *parent)
:QMainWindow(parent)
{
	// Menu
	QToolBar *menutoolbar = addToolBar("Menu");
	menu = new QMenuBar(menutoolbar);
	menutoolbar->addWidget(menu);


	// Vim shell
	vimshell = new QVimShell( gui, this );
	setCentralWidget(vimshell);
	vimshell->setFocus();

	// TabLine
	tabtoolbar = addToolBar("tabline");

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

void MainWindow::closeEvent (QCloseEvent * event)
{
	vimshell->closeEvent(event);
}

void MainWindow::showTabline(bool show)
{
	//tabdock->setVisible(show);
	tabtoolbar->setVisible(show);
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
