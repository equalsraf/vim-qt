#include "qvimshell.moc"

extern "C" {
#include "vim.h"
}

QVimShell::QVimShell(QWidget *parent)
:QGraphicsView(parent), m_foreground(QBrush(Qt::black))
{
	setCacheMode(QGraphicsView::CacheBackground);
	setScene(new QGraphicsScene(this));

//	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}


void QVimShell::setBackground(const QColor& color)
{
	setBackgroundBrush(QBrush(color));
	update();
}

void QVimShell::setForeground(const QColor& color)
{
	m_foreground = QBrush(color);
}

void QVimShell::clearAll()
{
	scene()->clear();
}

void QVimShell::clearBlock(int row1, int col1, int row2, int col2)
{
	qDebug() << "clearBlock" << row1 << col1 << row2 << col2;

	QPointF tl = mapToScene(FILL_X(col1), FILL_Y(row1) );
	QPointF br = mapToScene(FILL_X(col2+1), FILL_Y(row2+1) );

	qDebug() << "  " << QRectF(tl, br);
	QPainterPath path;
	path.addRect( QRectF(tl, br) );
	scene()->setSelectionArea(path);

	QListIterator<QGraphicsItem *> it(scene()->selectedItems());
	while(it.hasNext()) {
		QGraphicsItem *item = it.next();
		scene()->removeItem( item );
		qDebug() << "  item";
	}
}


void QVimShell::drawString(int row, int col, const QString& str, int flags)
{
	QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem( str );
	item->setBrush(m_foreground);

	QFont f = item->font();
	if (flags & DRAW_BOLD) {
		f.setBold(true);
	}
	if (flags & DRAW_UNDERL) {
		f.setUnderline(true);
	}
	item->setFont( f );

	scene()->addItem(item);
	item->setPos(mapToScene(TEXT_X(col), TEXT_Y(row) ));

}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	gui_resize_shell(ev->size().width(), ev->size().height());
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	//QGraphicsView::keyPressEvent(ev);
	
	if ( ev->text() != "" ) {
		add_to_input_buf( (char_u *) ev->text().constData(), ev->count() );
		return;
	} 

	
}


