#include "scrollarea.moc"

/**
 * The scroll area is a 3x3 grid to place
 * multipla scrollbars around a central widget
 *
 * ----------------------
 * |  |     North     |  |
 * |--|--------------|---|
 * |W |               |E |
 * |e |               |a |
 * |s |               |s |
 * |t |               |t |
 * |--|---------------|--|
 * |  |    South      |  |
 * |__|_______________|__|
 *
 * Each edge is a vertical/horizontal
 * box layout.
 */
ScrollArea::ScrollArea(QWidget *parent)
:m_widget(0)
{
	// Grid layout
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);

	// Edge layouts
	north = new QHBoxLayout();
	north->setSpacing(0);
	south = new QHBoxLayout();
	south->setSpacing(0);
	west = new QVBoxLayout();
	west->setSpacing(0);
	east = new QVBoxLayout();
	east->setSpacing(0);

	m_layout->addLayout(north, 0, 1);
	m_layout->addLayout(south, 2, 1);
	m_layout->addLayout(west, 1, 0);
	m_layout->addLayout(east, 1, 2);
}

void ScrollArea::setWidget(QWidget *w)
{
	m_widget = w;
	m_layout->addWidget(m_widget, 1, 1);
}

void ScrollArea::layoutEdge(QBoxLayout *edge)
{
	QLayoutItem *item;
	QLayoutItem *w;
	QList<VimScrollBar*> bars;
	int i;

	while ( (item = edge->takeAt(0)) ) {
		bars.append( static_cast<VimScrollBar*>(item->widget()) );
		delete item;
	}

	qSort(bars);
	foreach( VimScrollBar *b, bars ) {
		edge->addWidget(b);
	}
}

void ScrollArea::layoutEast()
{
	layoutEdge(east);
}

void ScrollArea::layoutWest()
{
	layoutEdge(west);
}

void ScrollArea::layoutSouth()
{
	layoutEdge(south);
}

void ScrollArea::addScrollbarRight(VimScrollBar *b)
{
	qDebug() << __func__ << b;
	connect(b, SIGNAL(indexChanged(int)),
			this, SLOT(layoutEast()));
	connect(b, SIGNAL(visibilityChanged(bool)),
			this, SLOT(layoutEast()));
	east->addWidget(b);
	layoutEdge(east);
}

void ScrollArea::addScrollbarBottom(VimScrollBar *b)
{
	connect(b, SIGNAL(indexChanged(int)),
			this, SLOT(layoutSouth()));
	connect(b, SIGNAL(visibilityChanged(bool)),
			this, SLOT(layoutSout()));
	south->addWidget(b);
	layoutEdge(south);
}

void ScrollArea::addScrollbarLeft(VimScrollBar *b)
{
	connect(b, SIGNAL(indexChanged(int)),
			this, SLOT(layoutWest()));
	connect(b, SIGNAL(visibilityChanged(bool)),
			this, SLOT(layoutWest()));
	west->addWidget(b);
	layoutEdge(west);
}

