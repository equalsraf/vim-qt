#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}


QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QGraphicsView(parent), m_foreground(QBrush(Qt::black)), m_gui(gui), 
	blinkState(BLINK_NONE), m_blinkWaitTime(700), m_blinkOnTime(400), m_blinkOffTime(250)
{

	setCacheMode(QGraphicsView::CacheBackground);
	setScene(new QGraphicsScene(this));

	connect(&blinkTimer, SIGNAL(timeout()),
			this, SLOT(blinkEvent()));

//	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	
	

	updateSettings();
}

void QVimShell::updateSettings()
{
	setBackgroundBrush(QBrush( *(m_gui->back_pixel) ));
	update();
}

void QVimShell::setBackground(const QColor& color)
{
	m_background = QBrush(color);
}

void QVimShell::setForeground(const QColor& color)
{
	m_foreground = QBrush(color);
}

void QVimShell::setSpecial(const QColor& color)
{
	m_special = QBrush(color);
}

void QVimShell::clearAll()
{
	scene()->clear();
}

void QVimShell::clearBlock(int row1, int col1, int row2, int col2)
{
	QPointF tl = QPointF(FILL_X(col1), FILL_Y(row1) );
	QPointF br = QPointF(FILL_X(col2+1), FILL_Y(row2+1) );

	// -- Debug
	static QGraphicsRectItem *last = NULL;
	if ( last != NULL ) {
		scene()->removeItem(last);
	}
	last = scene()->addRect(QRectF(tl, br), QPen(Qt::red));
	// -------

	QPainterPath path;
	path.addRect( QRectF(tl, br) );
	scene()->setSelectionArea(path);

	QListIterator<QGraphicsItem *> it(scene()->selectedItems());
	while(it.hasNext()) {
		QGraphicsItem *item = it.next();
		scene()->removeItem( item );
	}
}




void QVimShell::drawString(int row, int col, const QString& str, int flags)
{
	QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem( str );

	if ( m_foreground.color().isValid() ) {
		item->setBrush(m_foreground);
	} else {
		item->setBrush( *(m_gui->norm_pixel) );
	}

	QFont f = QFont( *(m_gui->norm_font) );
	if (flags & DRAW_BOLD) {
		f.setBold(true);
	}
	if (flags & DRAW_UNDERL) {
		f.setUnderline(true);
	}
	if (flags & DRAW_ITALIC) {
		f.setItalic(true);
	}

	item->setFont( f );
	item->setFlags( QGraphicsItem::ItemIsSelectable);


	// Draw background box

	if (flags & DRAW_TRANSP) {
		// Do we need to do anything
	} else if (   m_background.color().isValid() ) {
		QGraphicsRectItem *back = scene()->addRect(item->boundingRect(), QPen(Qt::NoPen), m_background);
		back->setPos( mapText(row, col) );
		back->setFlags( QGraphicsItem::ItemIsSelectable);
	}

	// -- Debug
	static QGraphicsRectItem *last = NULL;
	if ( last != NULL ) {
		scene()->removeItem(last);
	}
	last = scene()->addRect( item->boundingRect(), QPen(Qt::blue));
	// -------


	item->setPos( mapText(row, col) );

	scene()->addItem(item);
}

QPoint QVimShell::mapText(int row, int col)
{
	return QPoint( m_gui->char_width*col, m_gui->char_height*row );
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

void QVimShell::closeEvent(QCloseEvent *event)
{
	gui_shell_closed();
	event->ignore();
}

void QVimShell::drawPartCursor(const QColor& color, int w, int h)
{
	//qDebug() << __func__ << h << m_gui->char_height-h;

	QPoint tl = QPoint(FILL_X(m_gui->col+1), 
			FILL_Y(m_gui->row) + m_gui->char_height-h );

	QPoint br = QPoint(FILL_X(m_gui->col)+w, 
			FILL_Y(m_gui->row) + gui.char_height);

	QRect rect(tl, br);

	QGraphicsRectItem *back = scene()->addRect( rect, QPen(Qt::NoPen), QBrush(color));
	back->setFlags( QGraphicsItem::ItemIsSelectable);

}

void QVimShell::drawHollowCursor(const QColor& color)
{
	int w = m_gui->char_width;
	int h = m_gui->char_height;

	QPoint tl = QPoint(FILL_X(m_gui->col), 
			FILL_Y(m_gui->row) + m_gui->char_height-h );
	QPoint br = QPoint(FILL_X(m_gui->col)+w, 
			FILL_Y(m_gui->row)+gui.char_height);

	QRect rect(tl, br);

	QGraphicsRectItem *back = scene()->addRect( rect, QPen(Qt::red));
	back->setFlags( QGraphicsItem::ItemIsSelectable);

}

long QVimShell::blinkWaitTime()
{
	return m_blinkWaitTime;
}

long QVimShell::blinkOnTime()
{
	return m_blinkOnTime;
}

long QVimShell::blinkOffTime()
{
	return m_blinkOffTime;
}
void QVimShell::setBlinkWaitTime(long t)
{
	m_blinkWaitTime = t;
}
void QVimShell::setBlinkOnTime(long t)
{
	m_blinkOnTime = t;
}
void QVimShell::setBlinkOffTime(long t)
{
	m_blinkOffTime = t;
}

void QVimShell::startBlinking()
{
	/*
	if ( blinkState != BLINK_NONE ) {
		return;
	}

	qDebug() << __func__;
	blinkState = BLINK_ON;
	blinkTimer.start(blinkWaitTime());
	gui_update_cursor(TRUE, FALSE);
	*/
}

void QVimShell::stopBlinking()
{
	blinkTimer.stop();
	blinkState = BLINK_NONE;
	if ( blinkState == BLINK_OFF ) {
		gui_update_cursor(TRUE, FALSE);
	}
}

void QVimShell::blinkEvent()
{
	// FIXME
	if ( blinkState == BLINK_ON ) {
		gui_undraw_cursor();
		blinkState = BLINK_OFF;
		blinkTimer.start(blinkOnTime());
		qDebug() << __func__<< "OFF";

		//QTimer::singleShot( blinkOnTime(), this, SLOT(blinkEvent()));
	} else if (blinkState == BLINK_OFF ) {
		//QTimer::singleShot( blinkOffTime(), this, SLOT(blinkEvent()));
		gui_update_cursor(TRUE, FALSE);
		blinkState = BLINK_ON;
		blinkTimer.start(blinkOffTime());
		qDebug() << __func__<< "ON";

	}
}
