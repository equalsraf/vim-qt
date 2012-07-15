#include "qvimshell.moc"

#include <QResizeEvent>
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTimer>

extern "C" {
#include "proto/gui.pro"
}

#include "colortable.h"

QVimShell::QVimShell(QWidget *parent)
:QWidget(parent), m_encoding_utf8(true),
	m_lastClickEvent(-1), m_tooltip(0), m_slowStringDrawing(false),
	m_mouseHidden(false)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// cursor blinking
	timer_cursorBlinkOn = new QTimer();
	timer_cursorBlinkOff = new QTimer();
	timer_firstOff = new QTimer();
	timer_firstOn = new QTimer();
	timer_firstOn->setSingleShot(true);
	timer_firstOff->setSingleShot(true);
	blinkState = BLINK_NONE;
	connect(timer_cursorBlinkOn, SIGNAL(timeout()), this, SLOT(cursorOn()));
	connect(timer_firstOn, SIGNAL(timeout()), this, SLOT(startBlinkOnTimer()));
	connect(timer_firstOff, SIGNAL(timeout()), this, SLOT(startBlinkOffTimer()));
	connect(timer_cursorBlinkOff, SIGNAL(timeout()), this, SLOT(cursorOff()));

	// IM Tooltip
	m_tooltip = new QLabel(this);
	m_tooltip->setVisible(false);
	m_tooltip->setTextFormat(Qt::PlainText);
	m_tooltip->setTextInteractionFlags(Qt::NoTextInteraction);
	m_tooltip->setAutoFillBackground(true);

	// Widget Attributes
	setAttribute(Qt::WA_KeyCompression, true);
	setAttribute(Qt::WA_InputMethodEnabled, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAcceptDrops(true);
	setMouseTracking(true);
}

void QVimShell::setBackground(const QColor color)
{
	m_background = color;
	emit backgroundColorChanged(m_background);
}

void QVimShell::switchTab(int idx)
{
	vim.sendTablineEvent(idx);
}

void QVimShell::closeTab(int idx)
{
	vim.sendTablineMenuEvent(idx, TABLINE_MENU_CLOSE);
}


QColor QVimShell::background()
{
	return m_background;
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	if ( canvas.isNull() ) {
		QPixmap newCanvas = QPixmap( ev->size() );
		newCanvas.fill(background());
		canvas = newCanvas;
	} else {
		// Keep old contents
		QPixmap old = canvas.copy(QRect(QPoint(0,0), ev->size()));
		canvas = QPixmap( ev->size() );
		canvas.fill(background()); // FIXME: please optimise me

		{
		QPainter p(&canvas);
		p.drawPixmap(QPoint(0,0), old);
		}
	}

	update();
	vim.guiResizeShell(ev->size().width(), ev->size().height());
}

/**
 * Encode special keys into something we can pass
 * Vim through add_to_input_buf.
 *
 */
bool QVimShell::specialKey(QKeyEvent *ev, char* str, int *len)
{
	int i;
	int start = *len;
	for ( i=0; special_keys[i].key_sym != 0; i++ ) {
		if ( special_keys[i].key_sym == ev->key() ) {

			// Modifiers
			int_u vmod = vimKeyboardModifiers(QApplication::keyboardModifiers());

			int key_char;
			if (special_keys[i].code1 == NUL) {
				key_char = special_keys[i].code0;
			} else {
				key_char = TO_SPECIAL(special_keys[i].code0, special_keys[i].code1);
			}

			key_char = simplify_key(key_char, (int *)&vmod);
			if (key_char == CSI) {
				key_char = K_CSI;
			}

			if ( vmod ) {
				str[start++] = (char)CSI;
				str[start++] = (char)KS_MODIFIER;
				str[start++] = (char)vmod;
			}

			if (IS_SPECIAL(key_char)) {
				str[start++] = (char)CSI;
				str[start++] = K_SECOND(key_char);
				str[start++] = K_THIRD(key_char);
			} else {
				QByteArray key = VimWrapper::convertTo(ev->text());
				int j;
				for (j=0; j<key.size(); j++) {
					str[start++] = key[j];
				}
			}

			*len = start;
			return true;
		}
	}
	return false;
}

int_u QVimShell::vimKeyboardModifiers(Qt::KeyboardModifiers mod)
{
	int_u vim = 0x00;

	if ( mod & Qt::ShiftModifier ) {
		vim |= MOD_MASK_SHIFT;
	} 
	if ( mod & Qt::ControlModifier ) {
		vim |= MOD_MASK_CTRL;
	}
	if ( mod & Qt::AltModifier ) {
		vim |= MOD_MASK_ALT;
	}

	return vim;
}

int_u QVimShell::vimMouseModifiers(Qt::KeyboardModifiers mod)
{
	int_u vim = 0x00;

	if ( mod & Qt::ShiftModifier ) {
		vim |= MOUSE_SHIFT;
	} 
	if ( mod & Qt::ControlModifier ) {
		vim |= MOUSE_CTRL;
	}
	if ( mod & Qt::AltModifier ) {
		vim |= MOUSE_ALT;
	}

	return vim;
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	char str[20];
	int len=0;

	if ( specialKey( ev, str, &len)) {
		add_to_input_buf((char_u *) str, len);
	} else if ( !ev->text().isEmpty() ) {
		if ( QApplication::keyboardModifiers() == Qt::AltModifier ) {
			char str[2] = {(char)195, ev->text().data()[0].toAscii() + 64};
			add_to_input_buf( (char_u *) str, 2 );
		} else {
			add_to_input_buf( (char_u *) VimWrapper::convertTo(ev->text()).data(), ev->count() );
		}
	}

	// mousehide - conceal mouse pointer when typing
	if (p_mh && !m_mouseHidden ) {
		QApplication::setOverrideCursor(Qt::BlankCursor);
		m_mouseHidden = true;
	}
}

void QVimShell::close()
{
	vim.guiShellClosed();
}

void QVimShell::closeEvent(QCloseEvent *event)
{
	close();
	event->ignore();
}

/*
 * @Deprecated
 *
 * Either by sheer absurdity or font substitution magic, some
 * monospace fonts end up having different widths for each
 * style (regular, bold, etc). This causes text painting to
 * misspaint - particularly when painting a selection -  because
 * the text will be too wide to fit in its place..
 *
 * This obscure piece of code tries to find a pointSize for the
 * same font that respects the monospace char width. Hopefully
 * you have a decent monospace font and this is never called!!
 *
 */
QFont QVimShell::fixPainterFont( const QFont& pfont )
{
	QFontMetrics fm(pfont);

	if ( fm.averageCharWidth() != charWidth() ) {
		qDebug() << __func__ << "Font size mismatch a.k.a. this is not a proper monospace font";

		int V = (fm.averageCharWidth() > charWidth() ) ? -1 :1;
		int newsize;

		QFont f1 = pfont;
		newsize = f1.pointSize()-V;

		if ( newsize < 0 ) {
			return pfont;
		}
		f1.setPointSize(newsize);
		QFontMetrics fm1(f1);

		float wdiff = ((float)fm1.averageCharWidth() - fm.averageCharWidth())*V;
		int pt = (fm.averageCharWidth() - charWidth())/wdiff;

		QFont f = pfont;
		newsize = f.pointSize()+pt;
		if ( newsize < 0 ) {
			return pfont;
		}
		f.setPointSize(newsize);

		return f;
	}

	return pfont;
}

/*
 * Slow text painting strategy
 *
 * - Paints one character at a time
 * - Looks perfect in any condition
 * - Takes a looong time
 * - FIXME: add support for proper undercurl
 */
void QVimShell::drawStringSlow( const PaintOperation& op, QPainter &painter )
{
	QFont f = op.font;
	painter.setFont(f);
	painter.setPen( op.color );

	QRect rect = op.rect;
	foreach(QChar c, op.str) {
		if ( VimWrapper::charCellWidth(c) == 1 ) {
			rect.setWidth(VimWrapper::charWidth());
		} else if (VimWrapper::charCellWidth(c) == 2) {
			rect.setWidth(2*VimWrapper::charWidth());
		} else {
			qDebug() << __func__ << "invalid lenght" << c << VimWrapper::charCellWidth(c);
			continue;
		}

		QPoint pos = op.pos;
		pos.setX(rect.left());
		painter.drawText(pos, c);
		rect.moveTo( rect.x() + rect.width(), rect.y() );
	}
}

/**
 * Draw a string into the canvas
 *
 */
void QVimShell::drawString( const PaintOperation& op, QPainter &painter)
{
	painter.setPen( op.color );
	painter.setFont(op.font);
	painter.drawText( op.pos, op.str);
}

void QVimShell::flushPaintOps()
{
	QPainter painter(&canvas);
	while ( !paintOps.isEmpty() ) {
		painter.save();

		PaintOperation op = paintOps.dequeue();
		switch( op.type ) {
		case CLEARALL:
			painter.fillRect(canvas.rect(), op.color);
			break;
		case FILLRECT:
			painter.fillRect(op.rect, op.color);
			break;
		case DRAWRECT:
			painter.setPen(op.color);
			painter.drawRect(op.rect);
			break;
		case DRAWSTRING:
			painter.setClipRect(op.rect);

			// Disable underline if undercurl is in place
			if (op.undercurl && op.font.underline()) {
				op.font.setUnderline(false);
			}

			if ( m_slowStringDrawing ) {
				drawStringSlow(op, painter);
			} else if ( op.str.length() != VimWrapper::stringCellWidth(op.str) ) {
				drawStringSlow(op, painter);
			} else {
				drawString(op, painter);
			}

			// Draw undercurl
			// FIXME: we are doing it wrong - the underlinePos needs
			// to be stored someplace else
			if (op.undercurl) {
				QPoint start(op.rect.bottomLeft());
				start.setY(op.pos.y() + 1 + gui.char_ul_pos );
				QPoint end(start);
				end.setX(op.rect.right());

				QPen pen(op.curlcolor, 1, Qt::DashDotDotLine);
				painter.setPen(pen);
				painter.drawLine(QLine(start, end));
			}

			break;
		case DRAWSIGN:
			painter.drawPixmap( op.pos, op.sign);
			break;
		case INVERTRECT:
			painter.setCompositionMode( QPainter::RasterOp_SourceXorDestination );
			painter.fillRect( op.rect, Qt::color0);
			painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
			break;
		case SCROLLRECT:
			painter.restore();
			painter.end();

			QRegion exposed;
			canvas.scroll(op.pos.x(), op.pos.y(),
			 	op.rect, &exposed);

			painter.begin(&canvas);
			painter.fillRect(exposed.boundingRect(), op.color);
			continue; // exception, skip painter restore
		}

		painter.restore();
	}
}


void QVimShell::paintEvent ( QPaintEvent *ev )
{
	flushPaintOps();

	QPainter realpainter(this);
	foreach(const QRect r, ev->region().rects()) {
		realpainter.drawPixmap( r, canvas, r);
	}
}

//
// Mouse events
// FIXME: not handling modifiers

void QVimShell::mouseMoveEvent(QMouseEvent *ev)
{
	// mousehide - show mouse pointer
	restoreCursor();

	if ( ev->buttons() ) {
		int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());
		vim.guiSendMouseEvent(MOUSE_DRAG, ev->pos().x(),
					  ev->pos().y(), FALSE, vmod);
	} else {
		vim.guiMouseMoved(ev->pos().x(), ev->pos().y());
	}
}

void QVimShell::mousePressEvent(QMouseEvent *ev)
{
	int but;

	// mousehide - show mouse pointer
	restoreCursor();

	if ( !hasFocus() ) {
		setFocus(Qt::MouseFocusReason);
	}

	switch( ev->button() ) {
	case Qt::LeftButton:
		but = MOUSE_LEFT;
		break;
	case Qt::RightButton:
		but = MOUSE_RIGHT;
		break;
	case Qt::MidButton:
		but = MOUSE_MIDDLE;
		break;
	default:
		return;
	}

	int repeat=0;

	if ( !m_lastClick.isNull() 
		&& m_lastClick.elapsed() < QApplication::doubleClickInterval() 
		&& m_lastClickEvent == ev->button() ) {
		repeat = 1;
	}

	m_lastClick.restart();
	m_lastClickEvent = ev->button();

	int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());

	vim.guiSendMouseEvent(but, ev->pos().x(),
					  ev->pos().y(), repeat, vmod);
}

void QVimShell::mouseReleaseEvent(QMouseEvent *ev)
{
	int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());
	vim.guiSendMouseEvent(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, vmod);
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());
	vim.guiSendMouseEvent((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
					    ev->pos().x(), ev->pos().y(), FALSE, vmod);
}

/**
 * Get a color by name
 *
 * The color name can be any Vim or Qt color including html #colors.
 * Color names are case and space insensitive, i.e. "Dark Blue" 
 * and "darkblue" are the same color.
 *
 */
QColor QVimShell::color(const QString& name)
{
	QString cname = name.toLower().remove(' ');
	QColor c = ColorTable::get(cname, QColor());
	if ( !c.isValid() && cname != "transparent" ) {
		c.setNamedColor(cname);
	}

	return c;
}

void QVimShell::queuePaintOp(PaintOperation op)
{
	paintOps.enqueue(op);
	if ( op.rect.isValid() ) {
		update(op.rect);
	} else {
		update();
	}
}

void QVimShell::setEncodingUtf8(bool enabled)
{
	m_encoding_utf8 = enabled;
}

void QVimShell::dragEnterEvent(QDragEnterEvent *ev)
{
	if ( ev->mimeData()->hasFormat("text/uri-list") ||
		ev->mimeData()->hasFormat("text/html") ||
		ev->mimeData()->hasFormat("UTF8_STRING") ||
		ev->mimeData()->hasFormat("STRING") ||
	  	ev->mimeData()->hasFormat("text/plain") ) {
		ev->acceptProposedAction();
	}
}

void QVimShell::dropEvent(QDropEvent *ev)
{

	if ( ev->mimeData()->hasFormat("text/uri-list") ) {
		QList<QUrl> urls = ev->mimeData()->urls();
		if ( urls.size() == 0 ) {
			return;
		}
		vim.guiHandleDrop(ev->pos().x(), ev->pos().y(), 0, urls);

	} else {
		QByteArray text = VimWrapper::convertTo(ev->mimeData()->text());
		dnd_yank_drag_data( (char_u*)text.data(), text.size());

		char_u buf[3] = {CSI, KS_EXTRA, (char_u)KE_DROP};
		add_to_input_buf(buf, 3);
	}
	ev->acceptProposedAction();
}

void QVimShell::focusInEvent(QFocusEvent *ev)
{
	// mousehide - show mouse pointer
	restoreCursor();

	vim.guiFocusChanged(TRUE);
	QWidget::focusInEvent(ev);
	update();
}

void QVimShell::leaveEvent(QEvent *ev)
{
	restoreCursor();
	QWidget::leaveEvent(ev);
}

void QVimShell::enterEvent(QEvent *ev)
{
	restoreCursor();
	QWidget::leaveEvent(ev);
}

void QVimShell::focusOutEvent(QFocusEvent *ev)
{
	vim.guiFocusChanged(FALSE);
	QWidget::focusOutEvent(ev);
	update();
}

bool QVimShell::focusNextPrevChild(bool next)
{
	return false;
}

void QVimShell::setCharWidth(int w)
{
	m_charWidth = w;
}

int QVimShell::charWidth()
{
	return m_charWidth;
}

void QVimShell::inputMethodEvent(QInputMethodEvent *ev)
{
	if ( !ev->commitString().isEmpty() ) {
		QByteArray s = VimWrapper::convertTo(ev->commitString());
		add_to_input_buf_csi( (char_u *) s.data(), s.size() );
		tooltip("");
	} else {
		tooltip( ev->preeditString());
	}
}

QVariant QVimShell::inputMethodQuery(Qt::InputMethodQuery query)
{
	if ( query == Qt::ImFont) {
		return font();
	}

	return QVariant();
}

/*
 * Display a tooltip over the shell, covering underlying shell content.
 * The tooltip is placed at the current shell cursor position.
 *
 * When the given string is empty the tooltip is concealed.
 *
 * FIXME: Colors could use improving
 */
void QVimShell::tooltip(const QString& text)
{
	m_tooltip->setText(text);
	if ( text.isEmpty() ) {
		m_tooltip->hide();
		return;
	}

	if ( !m_tooltip->isVisible() ) {
		m_tooltip->setMinimumHeight(VimWrapper::charHeight());
		m_tooltip->move( VimWrapper::cursorPosition() );
		m_tooltip->show();
	}

	m_tooltip->setMinimumWidth( QFontMetrics(m_tooltip->font()).width(text) );
	m_tooltip->setMaximumWidth( QFontMetrics(m_tooltip->font()).width(text) );
	m_tooltip->update();
}

/*
 * If the cursor is invisible, make it visible again
 *
 */
void QVimShell::restoreCursor()
{
	QCursor *cursor = QApplication::overrideCursor();

	if ( cursor && m_mouseHidden ) {
		QApplication::restoreOverrideCursor();
		m_mouseHidden = false;
	}
}


void QVimShell::setBlinkTime(const long waittime, const long ontime, const long offtime)
{
	m_blinkWaitTime = waittime;
	m_blinkOnTime = ontime;
	m_blinkOffTime = offtime;
}

/*
 * start blinking: show cursor for waitingtime then blink
 */
void QVimShell::startBlinking()
{
	if (m_blinkWaitTime && m_blinkOnTime && m_blinkOffTime && hasFocus())
	{
		// wait waitTime before starting the blink
		timer_cursorBlinkOn->stop();
		timer_cursorBlinkOff->stop();
		timer_firstOff->stop();
		timer_firstOn->stop();

		timer_firstOff->start(m_blinkWaitTime);
		cursorOn();
	}
}

void QVimShell::startBlinkOnTimer()
{
	if (blinkState == BLINK_OFF) // if blinkstate == NONE do nothing
	{
		timer_cursorBlinkOn->start(m_blinkOnTime+m_blinkOffTime);
		cursorOn();
	}
}

void QVimShell::startBlinkOffTimer()
{
	if (blinkState == BLINK_ON) // if blinkstate == NONE do nothing
	{
		timer_firstOn->start(m_blinkOffTime);
		timer_cursorBlinkOff->start(m_blinkOnTime+m_blinkOffTime);
		cursorOff();
	}
}


/*
 * Stop cursor blinking
 */
void QVimShell::stopBlinking()
{
	blinkState = BLINK_NONE;
	timer_cursorBlinkOn->stop();
	timer_cursorBlinkOff->stop();
	timer_firstOn->stop();
	timer_firstOff->stop();
}

void QVimShell::cursorOff()
{
	blinkState = BLINK_OFF;
	vim.undrawCursor();
}

void QVimShell::cursorOn()
{
	blinkState = BLINK_ON;
	vim.updateCursor(true, false);
}
