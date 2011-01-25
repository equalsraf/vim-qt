#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}

struct special_key
{
	int key_sym;
	char code0;
	char code1;
};

static const struct special_key special_keys[] =
{
	{Qt::Key_Up,	'k', 'u'},
	{Qt::Key_Down,	'k', 'd'},
	{Qt::Key_Left,	'k', 'l'},
    {Qt::Key_Right,		'k', 'r'},
    {Qt::Key_F1,	'k', '1'},
    {Qt::Key_F2,	'k', '2'},
    {Qt::Key_F3,	'k', '3'},
    {Qt::Key_F4,	'k', '4'},
    {Qt::Key_F5,	'k', '5'},
    {Qt::Key_F6,		'k', '6'},
    {Qt::Key_F7,		'k', '7'},
    {Qt::Key_F8,		'k', '8'},
    {Qt::Key_F9,		'k', '9'},
    {Qt::Key_F10,		'k', ';'},
    {Qt::Key_F11,		'F', '1'},
    {Qt::Key_F12,		'F', '2'},
    {Qt::Key_F13,		'F', '3'},
    {Qt::Key_F14,		'F', '4'},
    {Qt::Key_F15,		'F', '5'},
    {Qt::Key_F16,		'F', '6'},
    {Qt::Key_F17,		'F', '7'},
    {Qt::Key_F18,		'F', '8'},
    {Qt::Key_F19,		'F', '9'},
    {Qt::Key_F20,		'F', 'A'},
    {Qt::Key_F21,		'F', 'B'},
    {Qt::Key_Pause,		'F', 'B'}, /* Pause == F21 according to netbeans.txt */
    {Qt::Key_F22,		'F', 'C'},
    {Qt::Key_F23,		'F', 'D'},
    {Qt::Key_F24,		'F', 'E'},
    {Qt::Key_F25,		'F', 'F'},
    {Qt::Key_F26,		'F', 'G'},
    {Qt::Key_F27,		'F', 'H'},
    {Qt::Key_F28,		'F', 'I'},
    {Qt::Key_F29,		'F', 'J'},
    {Qt::Key_F30,		'F', 'K'},
    {Qt::Key_F31,		'F', 'L'},
    {Qt::Key_F32,		'F', 'M'},
    {Qt::Key_F33,		'F', 'N'},
    {Qt::Key_F34,		'F', 'O'},
    {Qt::Key_F35,		'F', 'P'},
    {Qt::Key_Help,		'%', '1'},

    {Qt::Key_Backspace,	'k', 'b'},
    {Qt::Key_Insert,	'k', 'I'},
    {Qt::Key_Delete,	'k', 'D'},
    {Qt::Key_Backtab,	'k', 'B'},
    {Qt::Key_Clear,		'k', 'C'},
    {Qt::Key_Home,		'k', 'h'},
    {Qt::Key_End,		'@', '7'},
    /* Keypad keys: */

    /* End of list marker: */
    {0, 0, 0}
};


QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QGraphicsView(parent), m_foreground(QBrush(Qt::black)), m_gui(gui), 
	blinkState(BLINK_NONE), m_blinkWaitTime(700), m_blinkOnTime(400), m_blinkOffTime(250)
{

	setCacheMode(QGraphicsView::CacheBackground);
	//setAttribute( Qt::WA_PaintOnScreen );

	setScene(new QGraphicsScene(this));

	connect(&blinkTimer, SIGNAL(timeout()),
			this, SLOT(blinkEvent()));

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);

	scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	setAttribute(Qt::WA_KeyCompression, true);
	updateSettings();
}

void QVimShell::updateSettings()
{
	setBackgroundBrush(QBrush( *(m_gui->back_pixel) ));
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

	QFont f = m_font;
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
	QRectF br = item->boundingRect();


	if (flags & DRAW_TRANSP) {
		// Do we need to do anything?
	} else if (   m_background.color().isValid() ) {
		// Fill in the background
		QRectF bg = item->boundingRect();
		bg.setWidth( m_gui->char_width*str.length());
		QGraphicsRectItem *back = scene()->addRect( bg, QPen(Qt::NoPen), m_background);
		back->setPos( mapText(row, col) );
		back->setFlags( QGraphicsItem::ItemIsSelectable);
	}

	QPoint position = mapText(row, col);
	// Fix font position
	if ( str.length() == 1 ) {
		int x_fix = (m_gui->char_width*str.length() - br.width())/2;
		position.setX( position.x() + x_fix );
	}

	item->setPos( position );
	scene()->addItem(item);
}

QPoint QVimShell::mapText(int row, int col)
{
	return QPoint( m_gui->char_width*col, m_gui->char_height*row );
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	gui_resize_shell(ev->size().width(), ev->size().height());
	QGraphicsView::resizeEvent(ev);
}


unsigned int QVimShell::vimModifiers(Qt::KeyboardModifiers mod)
{
	if ( mod == Qt::NoModifier ) {
		return 0;
	}
	unsigned int vmod;
	if ( mod & Qt::ShiftModifier ) {
		vmod |= MOD_MASK_SHIFT;
	} if ( mod & Qt::ControlModifier ) {
		vmod |= MOD_MASK_CTRL;
	} if ( mod & Qt::AltModifier ) {
		vmod |= MOD_MASK_ALT;
	} if ( mod & Qt::MetaModifier ) {
		vmod |= MOD_MASK_META;
	}

	return vmod;
}

bool QVimShell::specialKey(int k, char str[3])
{
	int i;
	for ( i=0; special_keys[i].key_sym != 0; i++ ) {
		if ( special_keys[i].key_sym == k ) {
			str[0] = CSI;
			str[1] = special_keys[i].code0;
			str[2] = special_keys[i].code1;
			return true;
		}
	}
	return false;
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	//QGraphicsView::keyPressEvent(ev);
	if ( ev->text() != "" ) {
		add_to_input_buf( (char_u *) ev->text().constData(), ev->count() );
		return;
	} 

	char str[3];
	if ( specialKey( ev->key(), str)) {
		add_to_input_buf((char_u *) str, 3);
	}
}

void QVimShell::closeEvent(QCloseEvent *event)
{
	gui_shell_closed();
	event->ignore();
}

void QVimShell::drawPartCursor(const QColor& color, int w, int h)
{
	QRect rect( m_gui->col*m_gui->char_width,
			m_gui->row*m_gui->char_height,
			w, h);

	QGraphicsRectItem *back = NULL;
	//back = scene()->addRect( rect, QPen(Qt::NoPen), QBrush(color));
	//back->setFlags( QGraphicsItem::ItemIsSelectable);
}

void QVimShell::drawHollowCursor(const QColor& color)
{
	qDebug() << __func__;
	int w = m_gui->char_width;
	int h = m_gui->char_height;

	QPoint tl = QPoint(FILL_X(m_gui->col), 
			FILL_Y(m_gui->row) + m_gui->char_height-h );
	QPoint br = QPoint(FILL_X(m_gui->col)+w, 
			FILL_Y(m_gui->row)+gui.char_height);

	QRect rect(tl, br);
/*
	QGraphicsRectItem *back = scene()->addRect( rect, QPen(Qt::red));
	back->setFlags( QGraphicsItem::ItemIsSelectable);
*/
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

	} else if (blinkState == BLINK_OFF ) {
		gui_update_cursor(TRUE, FALSE);
		blinkState = BLINK_ON;
		blinkTimer.start(blinkOffTime());
		qDebug() << __func__<< "ON";

	}
}

void QVimShell::setFont(const QFont& font)
{
	m_font = font;
}

void QVimShell::deleteLines(int row, int num_lines)
{
	// Clear area
	clearBlock(row, gui.scroll_region_left, row+num_lines-1, gui.scroll_region_right);

	// Move 
	QPointF tl = mapText( row+num_lines, gui.scroll_region_left );
	QPoint br = mapText( m_gui->scroll_region_bot+1, m_gui->scroll_region_right+1);

	QPainterPath path;
	path.addRect( QRectF(tl, br) );
	scene()->setSelectionArea(path);

	QListIterator<QGraphicsItem *> it(scene()->selectedItems());
	while(it.hasNext()) {
		QGraphicsItem *item = it.next();
		item->moveBy( 0, -num_lines*m_gui->char_height); // FIXME
	}
}


void QVimShell::insertLines(int row, int num_lines)
{
	// Move 
	QPointF tl = mapText( m_gui->scroll_region_left,  row);
	QPointF br = mapText( m_gui->scroll_region_right+1, m_gui->scroll_region_bot + 1);

	QPainterPath path;
	path.addRect( QRectF(tl, br) );
	scene()->setSelectionArea(path);

	QListIterator<QGraphicsItem *> it(scene()->selectedItems());
	while(it.hasNext()) {
		QGraphicsItem *item = it.next();
		item->moveBy( 0, num_lines*m_gui->char_height); // FIXME
	}

	
	clearBlock( row, m_gui->scroll_region_left,
			row+num_lines-1, m_gui->scroll_region_right);

}
