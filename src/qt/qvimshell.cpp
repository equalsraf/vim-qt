#include "qvimshell.moc"

#include <QResizeEvent>
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QMimeData>

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
	setAttribute(Qt::WA_KeyCompression, false);
	setAttribute(Qt::WA_InputMethodEnabled, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_StaticContents, true);
	setAcceptDrops(true);
	setMouseTracking(true);

	PaintOperation op;
	op.type = CLEARALL;
	op.color = background();
	queuePaintOp(op);
}

void QVimShell::setBackground(const QColor color)
{
	m_background = color;
	emit backgroundColorChanged(m_background);
}

void QVimShell::switchTab(int idx)
{
	sendTablineEvent(idx);
}

void QVimShell::closeTab(int idx)
{
	sendTablineMenuEvent(idx, TABLINE_MENU_CLOSE);
}


QColor QVimShell::background()
{
	return m_background;
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	PaintOperation op;
	op.type = FILLRECT;
	op.color = background();

	int dWidth = ev->size().width() - ev->oldSize().width();
	if (dWidth > 0)
	{
		op.rect = QRect(ev->oldSize().width(), 0, dWidth, ev->size().height());
		queuePaintOp(op);
	}

	int dHeight = ev->size().height() - ev->oldSize().height();
	if (dHeight > 0)
	{
		op.rect = QRect(0, ev->oldSize().height(), ev->size().width(), dHeight);
		queuePaintOp(op);
	}

	//
	// Vim might trigger another resize, postpone the call
	// to guiResizeShell - otherwise we might be called
	// recursivelly and crash
	//
	postGuiResizeShell(ev->size().width(), ev->size().height());
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
	// mousehide - conceal mouse pointer when typing
	if (p_mh && !m_mouseHidden ) {
		QApplication::setOverrideCursor(Qt::BlankCursor);
		m_mouseHidden = true;
	}

	int key_char = 0;
	int vimModifiers = vimKeyboardModifiers(QApplication::keyboardModifiers());
	if ( !ev->text().isEmpty() ) {
		key_char = ev->text()[0].unicode(); // key compression is off
	}

	/* Find the special key */
	int i;
	bool isSpecial = false;
	for (i = 0; special_keys[i].key_sym != 0; ++i) {
		if (special_keys[i].key_sym == ev->key())
		{

			if (special_keys[i].code1 == NUL) {
				key_char = special_keys[i].code0;
			} else {
				key_char = TO_SPECIAL(special_keys[i].code0, special_keys[i].code1);
				key_char = simplify_key(key_char,
					(int *)&vimModifiers);
			}
			if (ev->key() != Qt::Key_Backtab) {
				isSpecial = TRUE;
			}
			break;
		}
	}

	if ( key_char == 0 ) {
		return;
	}

	/* TODO: Intercept CMD-. and CTRL-c ?*/

	//
	// For the most part this is same code as the one found in gui_mac.c, plus our
	// additional special keys bits, I've also kept the original comments because I
	// find them informative and slightly amusing
	//
	if (!isSpecial)
	{
		/* remove SHIFT for keys that are already shifted, e.g.,
		 * '(' and '*' */
		if (key_char < 0x100 && !isalpha(key_char) && isprint(key_char)) {
			vimModifiers &= ~MOD_MASK_SHIFT;
		}

		/* remove CTRL from keys that already have it */
		if (key_char < 0x20) {
			vimModifiers &= ~MOD_MASK_CTRL;
		}

		/* don't process unicode characters here */
		if (!IS_SPECIAL(key_char)) {
			/* Following code to simplify and consolidate vimModifiers
			 * taken liberally from gui_w48.c */
			key_char = simplify_key(key_char, (int *)&vimModifiers);

			/* Interpret META, include SHIFT, etc. */
			key_char = extract_modifiers(key_char, (int *)&vimModifiers);
			if (key_char == CSI) {
				key_char = K_CSI;
			}

			if (IS_SPECIAL(key_char)) {
				isSpecial = TRUE;
			}
		}
	}

	char_u result[3];
	if (vimModifiers) {
		result[0] = CSI;
		result[1] = KS_MODIFIER;
		result[2] = vimModifiers;
		add_to_input_buf(result, 3);
	}

	if (isSpecial && IS_SPECIAL(key_char)) {
		result[0] = CSI;
		result[1] = K_SECOND(key_char);
		result[2] = K_THIRD(key_char);
		add_to_input_buf(result, 3);
	} else {
		QByteArray utf8 = QString(QChar(key_char)).toUtf8();
		add_to_input_buf_csi( (char_u *) utf8.data(), utf8.size());
	}
}

void QVimShell::close()
{
	guiShellClosed();
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

void QVimShell::paintEvent ( QPaintEvent *ev )
{
	QPainter painter(this);
	while ( !paintOps.isEmpty() ) {
		painter.save();

		PaintOperation op = paintOps.dequeue();
		switch( op.type ) {
		case CLEARALL:
			painter.fillRect(this->rect(), op.color);
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

#if QT_VERSION >= 0x050000
			// From #56, in Qt5 scrolling does not work inside
			// a paint event, without overriding this attribute
			this->setAttribute(Qt::WA_WState_InPaintEvent, false);
#endif
			this->scroll(op.pos.x(), op.pos.y(), op.rect);
#if QT_VERSION >= 0x050000
			this->setAttribute(Qt::WA_WState_InPaintEvent);
#endif

			painter.begin(this);

			// Repaint exposed background. Vim won't redraw areas exposed by
			// scroll if it considers them empty because it assumes we already
			// cleared that area of the screen.
			QRect rect;
			rect.setWidth(op.rect.width());
			rect.setHeight(abs(op.pos.y()));
			if (op.pos.y() > 0)
				rect.moveTopLeft(op.rect.topLeft());
			else
				rect.moveBottomRight(op.rect.bottomRight());
			painter.fillRect(rect, op.color);

			continue; // exception, skip painter restore
		}

		painter.restore();
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
		guiSendMouseEvent(MOUSE_DRAG, ev->pos().x(),
					  ev->pos().y(), FALSE, vmod);
	} else {
		guiMouseMoved(ev->pos().x(), ev->pos().y());
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

	guiSendMouseEvent(but, ev->pos().x(),
					  ev->pos().y(), repeat, vmod);
}

void QVimShell::mouseReleaseEvent(QMouseEvent *ev)
{
	int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());
	guiSendMouseEvent(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, vmod);
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	int_u vmod = vimMouseModifiers(QApplication::keyboardModifiers());
	guiSendMouseEvent((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
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
		guiHandleDrop(ev->pos(), 0, urls);

	} else {
		guiHandleDropText(ev->mimeData()->text());
	}
	ev->acceptProposedAction();
}

void QVimShell::focusInEvent(QFocusEvent *ev)
{
	// mousehide - show mouse pointer
	restoreCursor();

	guiFocusChanged(TRUE);
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
	guiFocusChanged(FALSE);
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

QVariant QVimShell::inputMethodQuery(Qt::InputMethodQuery query) const
{
	if ( query == Qt::ImFont) {
		return font();
	} else if ( query == Qt::ImMicroFocus ) {
		return QRect(VimWrapper::cursorPosition(), QSize(0, charHeight()));
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
	undrawCursor();
}

void QVimShell::cursorOn()
{
	blinkState = BLINK_ON;
	updateCursor(true, false);
}
