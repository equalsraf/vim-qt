#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}


QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QWidget(parent), m_foreground(Qt::black), m_gui(gui), 
	blinkState(BLINK_NONE), m_blinkWaitTime(700), m_blinkOnTime(400), m_blinkOffTime(250)
{

	connect(&blinkTimer, SIGNAL(timeout()),
			this, SLOT(blinkEvent()));

	setAttribute(Qt::WA_KeyCompression, true);
	//setAttribute(Qt::WA_PaintOnScreen, true);
	updateSettings();
}

void QVimShell::updateSettings()
{
}

void QVimShell::setBackground(const QColor& color)
{
	m_background = color;
}

void QVimShell::setForeground(const QColor& color)
{
	m_foreground = color;
}

void QVimShell::setSpecial(const QColor& color)
{
	m_special = QBrush(color);
}

void QVimShell::clearAll()
{
	QPixmap *target = &pixmap;

	QPainter p(target);
	p.fillRect(target->rect(), *(m_gui->back_pixel));
}

QPoint QVimShell::mapText(int row, int col)
{
	return QPoint( m_gui->char_width*col, m_gui->char_height*row );
}

QRect QVimShell::mapBlock(int row1, int col1, int row2, int col2)
{
	QPoint tl = mapText( row1, col1 );
	QPoint br = mapText( row2+1, col2+1);
	br.setX( br.x()-1 );
	br.setY( br.y()-1 );

	return QRect(tl, br);
}

void QVimShell::clearBlock(int row1, int col1, int row2, int col2)
{
	QPainter p(&pixmap);
	QRect rect = mapBlock(row1, col1, row2, col2); 
	p.fillRect( rect, *(m_gui->back_pixel));
	update(rect);
}




void QVimShell::drawString(int row, int col, const QString& str, int flags)
{
	QPainter p(&pixmap);
	QPoint position = mapText(row, col);

	QRect rect( position.x(), position.y(), m_gui->char_width*str.length(), m_gui->char_height);

	if (flags & DRAW_TRANSP) {
		// Do we need to do anything?
	} else {
		// Fill in the background
		p.fillRect( QRect(position.x(), position.y(), 
					m_gui->char_width*str.length(), m_gui->char_height), m_background );
	}

	if ( m_foreground.isValid() ) {
		p.setPen(QPen(m_foreground));
	} else {
		p.setPen( QPen(*(m_gui->norm_pixel)) );
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

	p.setFont( f );
	QPoint textPosition = position;
	textPosition.setY( textPosition.y() + m_gui->char_height - 1 );
	p.drawText(rect, str);
	update(rect);
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	pixmap = QPixmap( ev->size() );

	{
	QPainter p(&pixmap);
	p.fillRect(pixmap.rect(), QBrush( *(m_gui->back_pixel) ));
	}

	gui_resize_shell(ev->size().width(), ev->size().height());
	update();
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

	char str[3];
	int modifiers = vimModifiers(ev->modifiers());
/*
	if ( modifiers ) {
		str[0] = CSI;
		str[1] = KS_MODIFIER;
		str[2] = modifiers;
		add_to_input_buf((char_u *) str, 3);
	}
*/
	if ( ev->text() != "" ) {
		add_to_input_buf( (char_u *) ev->text().constData(), ev->count() );
		return;
	} 

/*
	if ( specialKey( ev->key(), str)) {
		add_to_input_buf((char_u *) str, 3);
	}
*/
}

void QVimShell::closeEvent(QCloseEvent *event)
{
	gui_shell_closed();
	event->ignore();
}

void QVimShell::drawPartCursor(const QColor& color, int w, int h)
{
	QPainter p(&pixmap);

	QRect rect( m_gui->col*m_gui->char_width,
			m_gui->row*m_gui->char_height,
			w, h);

	p.fillRect(rect, m_foreground);
	update(rect);
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

	QPainter p(&pixmap);
	p.drawRect(rect);
	update(rect);
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
	QPoint tl = mapText( row+num_lines, gui.scroll_region_left );
	QPoint br = mapText( m_gui->scroll_region_bot+1, m_gui->scroll_region_right+1);
	QRect rect(tl, br);

	QPixmap tmp = pixmap.copy(rect);
	tmp.save("debug.jpg");
	QRect dest = rect;
	dest.setY( rect.y() - num_lines*m_gui->char_height );


	QPainter p(&pixmap);
	p.drawPixmap( tmp.rect(), tmp, tmp.rect());
	update( dest.united(rect) );
}

/*
 * @Warning
 *
 * Evidently mapText implies there will be breakage when scrolling
 *
 */

void QVimShell::insertLines(int row, int num_lines)
{
	// Move 
	QPoint tl = mapText( row, m_gui->scroll_region_left);
	QPoint br = mapText(m_gui->scroll_region_bot+1, m_gui->scroll_region_right+1);

	QRect rect(tl, br);

	QPixmap tmp = pixmap.copy(rect);

	tl.setY( tl.y() + num_lines*m_gui->char_height );
	QRect dest( tl, br);

	QRect src( QPoint(0,0),  mapText( m_gui->scroll_region_bot+1-num_lines , m_gui->scroll_region_right+1));
	
	QPainter p(&pixmap);
	p.drawPixmap( dest, tmp, src);
	p.end();

	clearBlock( row, m_gui->scroll_region_left,
			row+num_lines-1, m_gui->scroll_region_right);
	update(); // FIXME - we can do better

}


void QVimShell::invertRectangle(int row, int col, int nr, int nc)
{
}

void QVimShell::paintEvent ( QPaintEvent *ev )
{
	QPainter painter(this);
	painter.drawPixmap( ev->rect(), pixmap, ev->rect());
}
