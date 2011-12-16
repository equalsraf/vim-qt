#include "tabbar.moc"


TabBar::TabBar(QWidget *parent)
:QTabBar(parent)
{
}


void TabBar::mouseReleaseEvent(QMouseEvent *ev)
{
	if (ev->button() != Qt::MiddleButton) {
		QTabBar::mouseReleaseEvent(ev);
		return;
	}

	int tab = tabAt(ev->pos());
	if ( tab != -1 ) {
		emit QTabBar::tabCloseRequested(tab);
	}
}

