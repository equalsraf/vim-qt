#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}


QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QWidget(parent), m_foreground(Qt::black), m_gui(gui), 
	pixmap(1,1)
{
	pm_painter = new QPainter(&pixmap);
	setAttribute(Qt::WA_KeyCompression, true);
	setMouseTracking(true);
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
	pm_painter->fillRect(pixmap.rect(), *(m_gui->back_pixel));
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
	QRect rect = mapBlock(row1, col1, row2, col2); 
	pm_painter->fillRect( rect, *(m_gui->back_pixel));
	update(rect);
}




void QVimShell::drawString(int row, int col, const QString& str, int flags)
{
	QPoint position = mapText(row, col);

	QRect rect( position.x(), position.y(), m_gui->char_width*str.length(), m_gui->char_height);

	if (flags & DRAW_TRANSP) {
		// Do we need to do anything?
	} else {
		// Fill in the background
		pm_painter->fillRect( QRect(position.x(), position.y(), 
					m_gui->char_width*str.length(), m_gui->char_height), m_background );
	}

	if ( m_foreground.isValid() ) {
		pm_painter->setPen(QPen(m_foreground));
	} else {
		pm_painter->setPen( QPen(*(m_gui->norm_pixel)) );
	}

	QFont f = m_font;
	f.setBold( flags & DRAW_BOLD);
	f.setUnderline( flags & DRAW_UNDERL);
	f.setItalic( flags & DRAW_ITALIC);
	// FIXME: missing undercurl

	pm_painter->setFont( f );
	QPoint textPosition = position;
	textPosition.setY( textPosition.y() + m_gui->char_height - 1 );
	pm_painter->drawText(rect, str);
	update(rect);
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	pm_painter->end();
	free(pm_painter);

	pixmap = QPixmap( ev->size() );

	pm_painter = new QPainter(&pixmap);
	pm_painter->fillRect(pixmap.rect(), QBrush( *(m_gui->back_pixel) ));

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

// FIXME
QByteArray QVimShell::convert(const QString& s)
{
	QByteArray encodedString;

	if ( enc_utf8 ) {
		return s.toUtf8();
	} else {
		return s.toAscii();
	}
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	//QGraphicsView::keyPressEvent(ev);
	char str[3];

	if ( specialKey( ev->key(), str)) {
		add_to_input_buf((char_u *) str, 3);
		return;
	}

	/*
	int modifiers = vimModifiers(ev->modifiers());
	if ( modifiers ) {
		str[0] = CSI;
		str[1] = KS_MODIFIER;
		str[2] = modifiers;
		add_to_input_buf((char_u *) str, 3);
	}*/

	if ( !ev->text().isEmpty() ) {
		add_to_input_buf( (char_u *) convert(ev->text()).data(), ev->count() );
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
	QRect rect( m_gui->col*m_gui->char_width,
			m_gui->row*m_gui->char_height,
			w, h);

	pm_painter->fillRect(rect, m_foreground);
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

	pm_painter->drawRect(rect);
	update(rect);
}


void QVimShell::setFont(const QFont& font)
{
	m_font = font;
}

void QVimShell::deleteLines(int row, int num_lines)
{
	qDebug() << __func__;

	pm_painter->end();

	QRegion exposed;
	QRect scrollRect = mapBlock(row, m_gui->scroll_region_left, 
					m_gui->scroll_region_bot+1, m_gui->scroll_region_right+1);
	pixmap.scroll(0, -num_lines*m_gui->char_height,
			 scrollRect, &exposed);

	pm_painter->begin(&pixmap);

	// i.e. clearBlock
	pm_painter->fillRect( exposed.boundingRect(), *(m_gui->back_pixel));
	update( scrollRect );
}

void QVimShell::insertLines(int row, int num_lines)
{
	qDebug() << __func__;
	QRegion exposed;
	QRect scrollRect = mapBlock(row, m_gui->scroll_region_left, 
					m_gui->scroll_region_bot+1, m_gui->scroll_region_right+1);

	pm_painter->end();
	pixmap.scroll(0, num_lines*m_gui->char_height,
			 scrollRect, &exposed);

	pm_painter->begin(&pixmap);
	// i.e. clearBlock
	pm_painter->fillRect( exposed.boundingRect(), *(m_gui->back_pixel));
	update( scrollRect );
}


void QVimShell::invertRectangle(int row, int col, int nr, int nc)
{
	QRect rect = mapBlock(row, col, row+nr-1, col +nr-1);

	pm_painter->setCompositionMode( QPainter::CompositionMode_Difference );
	pm_painter->fillRect( rect, Qt::black);
	pm_painter->setCompositionMode( QPainter::CompositionMode_SourceOver );
}

void QVimShell::paintEvent ( QPaintEvent *ev )
{
	QPainter painter(this);
	painter.drawPixmap( ev->rect(), pixmap, ev->rect());
}

//
// Mouse events
// FIXME: not handling modifiers

void QVimShell::mouseMoveEvent(QMouseEvent *ev)
{	
	if ( ev->buttons() ) {
		gui_send_mouse_event(MOUSE_DRAG, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	} else {
		gui_mouse_moved(ev->pos().x(), ev->pos().y());
	}
}

void QVimShell::mousePressEvent(QMouseEvent *ev)
{
	int but;

	switch( ev->button() ) {
	case Qt::LeftButton:
		but = MOUSE_LEFT;
		break;
	case Qt::RightButton:
		but = MOUSE_RIGHT;
		break;
	case Qt::MiddleButton:
		but = MOUSE_MIDDLE;
		break;
	default:
		return;
	}

	gui_send_mouse_event(but, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
}

void QVimShell::mouseDoubleClickEvent(QMouseEvent *ev)
{
	gui_send_mouse_event(MOUSE_LEFT, ev->pos().x(),
					  ev->pos().y(), TRUE, 0);
}

void QVimShell::mouseReleaseEvent(QMouseEvent *ev)
{
	gui_send_mouse_event(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	gui_send_mouse_event((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
					    ev->pos().x(), ev->pos().y(), FALSE, 0);
}
