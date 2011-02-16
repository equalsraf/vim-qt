#include "mainwindow.moc"

MainWindow::MainWindow( gui_T* gui, QWidget *parent)
:QMainWindow(parent)
{

	vimshell = new QVimShell( gui, this );
	setCentralWidget(vimshell);
	vimshell->setFocus();
}

QVimShell* MainWindow::vimShell()
{
	return this->vimshell;
}

void MainWindow::closeEvent (QCloseEvent * event)
{
	vimshell->closeEvent(event);
}


