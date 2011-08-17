#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}

QHash<QString, QColor> QVimShell::m_colorMap;

QVimShell::QVimShell(QWidget *parent)
:QWidget(parent), m_encoding_utf8(true),
	m_lastClickEvent(-1), m_tooltip(0)
{
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
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAcceptDrops(true);
}

void QVimShell::setBackground(const QColor color)
{
	m_background = color;
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

bool QVimShell::isFakeMonospace(const QFont& f)
{
	QFontMetrics fm(f);
	return ( fm.averageCharWidth() != VimWrapper::charWidth() );
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
	f.setUnderline(op.undercurl);
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

		if ( c != ' ' ) {
			painter.drawText(rect, 
				Qt::TextSingleLine | Qt::AlignCenter | Qt::AlignTop, 
				c);
		}
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

	QTextLayout l(op.str);
	QTextOption opt;
	opt.setWrapMode(QTextOption::WrapAnywhere);

	l.setTextOption(opt);
	l.setFont(op.font);

	if (op.undercurl) {
		QTextCharFormat charFormat;
		charFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
		charFormat.setUnderlineColor(op.curlcolor);

		QTextLayout::FormatRange format;
		format.start = 0;
		format.length = op.str.size();
		format.format = charFormat;

		QList<QTextLayout::FormatRange> list;
		list.append(format);
		l.setAdditionalFormats(list);
	}

	l.beginLayout();
	QTextLine line = l.createLine();
	line.setNumColumns(op.str.size());
	l.endLayout();
	l.draw(&painter, op.rect.topLeft());

	//
	// An error of one pixel seems to be insignificant. We tolerate this here
	// but the painter is clipped anyway.
	int miss = l.maximumWidth() - QFontMetrics(op.font).width(op.str);
	if( miss  > 1 || miss < -1 ) {
		qDebug() << __func__ << "rect mismatch, this is serious, please poke a developer about this" 
			<< l.maximumWidth() << QFontMetrics(op.font).width(op.str) << op.str << op.str.size() << op.font;
	}
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
			if ( getenv("QVIM_DRAW_STRING_SLOW") || op.str.length() != VimWrapper::stringCellWidth(op.str) ) {
				drawStringSlow(op, painter);
			} else if ( isFakeMonospace(op.font) ) {
				qDebug() << __func__ << "Font size mismatch a.k.a. this is not a proper monospace font";
				drawStringSlow(op, painter);
			} else {
				drawString(op, painter);
			}

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
	if ( ev->buttons() ) {
		vim.guiSendMouseEvent(MOUSE_DRAG, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	} else {
		vim.guiMouseMoved(ev->pos().x(), ev->pos().y());
	}
}

void QVimShell::mousePressEvent(QMouseEvent *ev)
{
	int but;
	setMouseTracking(true);

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
	vim.guiSendMouseEvent(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	setMouseTracking(false);
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	vim.guiSendMouseEvent((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
					    ev->pos().x(), ev->pos().y(), FALSE, 0);
}

/**
 * Generate a color table from rgb.txt
 *
 */
void QVimShell::loadColors(const QString& name)
{
	QFile f(name);
	if (!f.open(QFile::ReadOnly)) {
		return;
	}

	while (!f.atEnd()) {
		QString line = VimWrapper::convertFrom( f.readLine() );
		if ( line.startsWith("!") ) {
			continue;
		}

		QStringList list = line.split( QRegExp("\\W+"), QString::SkipEmptyParts);
		if ( list.size() != 4 ) {
			continue;
		}

		int r,g,b;
		bool ok_r, ok_g, ok_b;

		r = list[0].toUInt(&ok_r);
		g = list[1].toUInt(&ok_g);
		b = list[2].toUInt(&ok_b);
		if ( !ok_r || !ok_g || !ok_b ) {
			continue;
		}

		QString name = list[3];
		printf("\tm_colorMap[\"%s\"] = QColor(%d, %d, %d);\n", name.toLower().toLatin1().constData(), r, g, b);
	}
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
	QColor c =m_colorMap.value(cname, QColor());
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
	vim.guiFocusChanged(TRUE);
	QWidget::focusInEvent(ev);
	update();
}

void QVimShell::focusOutEvent(QFocusEvent *ev)
{
	vim.guiFocusChanged(FALSE);
	QWidget::focusOutEvent(ev);
	update();
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
 * Mark the given clipboard has not owned by Vim
 *
 */
void QVimShell::clipboardChanged(QClipboard::Mode mode)
{
	switch(mode) {
	case QClipboard::Clipboard:
		clip_star.owned = FALSE;
		break;
	case QClipboard::Selection:
		clip_plus.owned = FALSE;
		break;
	}
}

/*
 * Fill color table with Vim colors
 */
void QVimShell::setupColorMap()
{
	/*
	 * For some reason these are not in rgb.txt
	 */
	m_colorMap["lightmagenta"] = QColor(255, 224, 255);
	m_colorMap["lightred"] = QColor(255, 0, 0);

	/*
	 * DONT EDIT - auto generated from rgb.txt
	 */
	m_colorMap["snow"] = QColor(255, 250, 250);
	m_colorMap["ghostwhite"] = QColor(248, 248, 255);
	m_colorMap["whitesmoke"] = QColor(245, 245, 245);
	m_colorMap["gainsboro"] = QColor(220, 220, 220);
	m_colorMap["floralwhite"] = QColor(255, 250, 240);
	m_colorMap["oldlace"] = QColor(253, 245, 230);
	m_colorMap["linen"] = QColor(250, 240, 230);
	m_colorMap["antiquewhite"] = QColor(250, 235, 215);
	m_colorMap["papayawhip"] = QColor(255, 239, 213);
	m_colorMap["blanchedalmond"] = QColor(255, 235, 205);
	m_colorMap["bisque"] = QColor(255, 228, 196);
	m_colorMap["peachpuff"] = QColor(255, 218, 185);
	m_colorMap["navajowhite"] = QColor(255, 222, 173);
	m_colorMap["moccasin"] = QColor(255, 228, 181);
	m_colorMap["cornsilk"] = QColor(255, 248, 220);
	m_colorMap["ivory"] = QColor(255, 255, 240);
	m_colorMap["lemonchiffon"] = QColor(255, 250, 205);
	m_colorMap["seashell"] = QColor(255, 245, 238);
	m_colorMap["honeydew"] = QColor(240, 255, 240);
	m_colorMap["mintcream"] = QColor(245, 255, 250);
	m_colorMap["azure"] = QColor(240, 255, 255);
	m_colorMap["aliceblue"] = QColor(240, 248, 255);
	m_colorMap["lavender"] = QColor(230, 230, 250);
	m_colorMap["lavenderblush"] = QColor(255, 240, 245);
	m_colorMap["mistyrose"] = QColor(255, 228, 225);
	m_colorMap["white"] = QColor(255, 255, 255);
	m_colorMap["black"] = QColor(0, 0, 0);
	m_colorMap["darkslategray"] = QColor(47, 79, 79);
	m_colorMap["darkslategrey"] = QColor(47, 79, 79);
	m_colorMap["dimgray"] = QColor(105, 105, 105);
	m_colorMap["dimgrey"] = QColor(105, 105, 105);
	m_colorMap["slategray"] = QColor(112, 128, 144);
	m_colorMap["slategrey"] = QColor(112, 128, 144);
	m_colorMap["lightslategray"] = QColor(119, 136, 153);
	m_colorMap["lightslategrey"] = QColor(119, 136, 153);
	m_colorMap["gray"] = QColor(190, 190, 190);
	m_colorMap["grey"] = QColor(190, 190, 190);
	m_colorMap["lightgrey"] = QColor(211, 211, 211);
	m_colorMap["lightgray"] = QColor(211, 211, 211);
	m_colorMap["midnightblue"] = QColor(25, 25, 112);
	m_colorMap["navy"] = QColor(0, 0, 128);
	m_colorMap["navyblue"] = QColor(0, 0, 128);
	m_colorMap["cornflowerblue"] = QColor(100, 149, 237);
	m_colorMap["darkslateblue"] = QColor(72, 61, 139);
	m_colorMap["slateblue"] = QColor(106, 90, 205);
	m_colorMap["mediumslateblue"] = QColor(123, 104, 238);
	m_colorMap["lightslateblue"] = QColor(132, 112, 255);
	m_colorMap["mediumblue"] = QColor(0, 0, 205);
	m_colorMap["royalblue"] = QColor(65, 105, 225);
	m_colorMap["blue"] = QColor(0, 0, 255);
	m_colorMap["dodgerblue"] = QColor(30, 144, 255);
	m_colorMap["deepskyblue"] = QColor(0, 191, 255);
	m_colorMap["skyblue"] = QColor(135, 206, 235);
	m_colorMap["lightskyblue"] = QColor(135, 206, 250);
	m_colorMap["steelblue"] = QColor(70, 130, 180);
	m_colorMap["lightsteelblue"] = QColor(176, 196, 222);
	m_colorMap["lightblue"] = QColor(173, 216, 230);
	m_colorMap["powderblue"] = QColor(176, 224, 230);
	m_colorMap["paleturquoise"] = QColor(175, 238, 238);
	m_colorMap["darkturquoise"] = QColor(0, 206, 209);
	m_colorMap["mediumturquoise"] = QColor(72, 209, 204);
	m_colorMap["turquoise"] = QColor(64, 224, 208);
	m_colorMap["cyan"] = QColor(0, 255, 255);
	m_colorMap["lightcyan"] = QColor(224, 255, 255);
	m_colorMap["cadetblue"] = QColor(95, 158, 160);
	m_colorMap["mediumaquamarine"] = QColor(102, 205, 170);
	m_colorMap["aquamarine"] = QColor(127, 255, 212);
	m_colorMap["darkgreen"] = QColor(0, 100, 0);
	m_colorMap["darkolivegreen"] = QColor(85, 107, 47);
	m_colorMap["darkseagreen"] = QColor(143, 188, 143);
	m_colorMap["seagreen"] = QColor(46, 139, 87);
	m_colorMap["mediumseagreen"] = QColor(60, 179, 113);
	m_colorMap["lightseagreen"] = QColor(32, 178, 170);
	m_colorMap["palegreen"] = QColor(152, 251, 152);
	m_colorMap["springgreen"] = QColor(0, 255, 127);
	m_colorMap["lawngreen"] = QColor(124, 252, 0);
	m_colorMap["green"] = QColor(0, 255, 0);
	m_colorMap["chartreuse"] = QColor(127, 255, 0);
	m_colorMap["mediumspringgreen"] = QColor(0, 250, 154);
	m_colorMap["greenyellow"] = QColor(173, 255, 47);
	m_colorMap["limegreen"] = QColor(50, 205, 50);
	m_colorMap["yellowgreen"] = QColor(154, 205, 50);
	m_colorMap["forestgreen"] = QColor(34, 139, 34);
	m_colorMap["olivedrab"] = QColor(107, 142, 35);
	m_colorMap["darkkhaki"] = QColor(189, 183, 107);
	m_colorMap["khaki"] = QColor(240, 230, 140);
	m_colorMap["palegoldenrod"] = QColor(238, 232, 170);
	m_colorMap["lightgoldenrodyellow"] = QColor(250, 250, 210);
	m_colorMap["lightyellow"] = QColor(255, 255, 224);
	m_colorMap["yellow"] = QColor(255, 255, 0);
	m_colorMap["gold"] = QColor(255, 215, 0);
	m_colorMap["lightgoldenrod"] = QColor(238, 221, 130);
	m_colorMap["goldenrod"] = QColor(218, 165, 32);
	m_colorMap["darkgoldenrod"] = QColor(184, 134, 11);
	m_colorMap["rosybrown"] = QColor(188, 143, 143);
	m_colorMap["indianred"] = QColor(205, 92, 92);
	m_colorMap["saddlebrown"] = QColor(139, 69, 19);
	m_colorMap["sienna"] = QColor(160, 82, 45);
	m_colorMap["peru"] = QColor(205, 133, 63);
	m_colorMap["burlywood"] = QColor(222, 184, 135);
	m_colorMap["beige"] = QColor(245, 245, 220);
	m_colorMap["wheat"] = QColor(245, 222, 179);
	m_colorMap["sandybrown"] = QColor(244, 164, 96);
	m_colorMap["tan"] = QColor(210, 180, 140);
	m_colorMap["chocolate"] = QColor(210, 105, 30);
	m_colorMap["firebrick"] = QColor(178, 34, 34);
	m_colorMap["brown"] = QColor(165, 42, 42);
	m_colorMap["darksalmon"] = QColor(233, 150, 122);
	m_colorMap["salmon"] = QColor(250, 128, 114);
	m_colorMap["lightsalmon"] = QColor(255, 160, 122);
	m_colorMap["orange"] = QColor(255, 165, 0);
	m_colorMap["darkorange"] = QColor(255, 140, 0);
	m_colorMap["coral"] = QColor(255, 127, 80);
	m_colorMap["lightcoral"] = QColor(240, 128, 128);
	m_colorMap["tomato"] = QColor(255, 99, 71);
	m_colorMap["orangered"] = QColor(255, 69, 0);
	m_colorMap["red"] = QColor(255, 0, 0);
	m_colorMap["hotpink"] = QColor(255, 105, 180);
	m_colorMap["deeppink"] = QColor(255, 20, 147);
	m_colorMap["pink"] = QColor(255, 192, 203);
	m_colorMap["lightpink"] = QColor(255, 182, 193);
	m_colorMap["palevioletred"] = QColor(219, 112, 147);
	m_colorMap["maroon"] = QColor(176, 48, 96);
	m_colorMap["mediumvioletred"] = QColor(199, 21, 133);
	m_colorMap["violetred"] = QColor(208, 32, 144);
	m_colorMap["magenta"] = QColor(255, 0, 255);
	m_colorMap["violet"] = QColor(238, 130, 238);
	m_colorMap["plum"] = QColor(221, 160, 221);
	m_colorMap["orchid"] = QColor(218, 112, 214);
	m_colorMap["mediumorchid"] = QColor(186, 85, 211);
	m_colorMap["darkorchid"] = QColor(153, 50, 204);
	m_colorMap["darkviolet"] = QColor(148, 0, 211);
	m_colorMap["blueviolet"] = QColor(138, 43, 226);
	m_colorMap["purple"] = QColor(160, 32, 240);
	m_colorMap["mediumpurple"] = QColor(147, 112, 219);
	m_colorMap["thistle"] = QColor(216, 191, 216);
	m_colorMap["snow1"] = QColor(255, 250, 250);
	m_colorMap["snow2"] = QColor(238, 233, 233);
	m_colorMap["snow3"] = QColor(205, 201, 201);
	m_colorMap["snow4"] = QColor(139, 137, 137);
	m_colorMap["seashell1"] = QColor(255, 245, 238);
	m_colorMap["seashell2"] = QColor(238, 229, 222);
	m_colorMap["seashell3"] = QColor(205, 197, 191);
	m_colorMap["seashell4"] = QColor(139, 134, 130);
	m_colorMap["antiquewhite1"] = QColor(255, 239, 219);
	m_colorMap["antiquewhite2"] = QColor(238, 223, 204);
	m_colorMap["antiquewhite3"] = QColor(205, 192, 176);
	m_colorMap["antiquewhite4"] = QColor(139, 131, 120);
	m_colorMap["bisque1"] = QColor(255, 228, 196);
	m_colorMap["bisque2"] = QColor(238, 213, 183);
	m_colorMap["bisque3"] = QColor(205, 183, 158);
	m_colorMap["bisque4"] = QColor(139, 125, 107);
	m_colorMap["peachpuff1"] = QColor(255, 218, 185);
	m_colorMap["peachpuff2"] = QColor(238, 203, 173);
	m_colorMap["peachpuff3"] = QColor(205, 175, 149);
	m_colorMap["peachpuff4"] = QColor(139, 119, 101);
	m_colorMap["navajowhite1"] = QColor(255, 222, 173);
	m_colorMap["navajowhite2"] = QColor(238, 207, 161);
	m_colorMap["navajowhite3"] = QColor(205, 179, 139);
	m_colorMap["navajowhite4"] = QColor(139, 121, 94);
	m_colorMap["lemonchiffon1"] = QColor(255, 250, 205);
	m_colorMap["lemonchiffon2"] = QColor(238, 233, 191);
	m_colorMap["lemonchiffon3"] = QColor(205, 201, 165);
	m_colorMap["lemonchiffon4"] = QColor(139, 137, 112);
	m_colorMap["cornsilk1"] = QColor(255, 248, 220);
	m_colorMap["cornsilk2"] = QColor(238, 232, 205);
	m_colorMap["cornsilk3"] = QColor(205, 200, 177);
	m_colorMap["cornsilk4"] = QColor(139, 136, 120);
	m_colorMap["ivory1"] = QColor(255, 255, 240);
	m_colorMap["ivory2"] = QColor(238, 238, 224);
	m_colorMap["ivory3"] = QColor(205, 205, 193);
	m_colorMap["ivory4"] = QColor(139, 139, 131);
	m_colorMap["honeydew1"] = QColor(240, 255, 240);
	m_colorMap["honeydew2"] = QColor(224, 238, 224);
	m_colorMap["honeydew3"] = QColor(193, 205, 193);
	m_colorMap["honeydew4"] = QColor(131, 139, 131);
	m_colorMap["lavenderblush1"] = QColor(255, 240, 245);
	m_colorMap["lavenderblush2"] = QColor(238, 224, 229);
	m_colorMap["lavenderblush3"] = QColor(205, 193, 197);
	m_colorMap["lavenderblush4"] = QColor(139, 131, 134);
	m_colorMap["mistyrose1"] = QColor(255, 228, 225);
	m_colorMap["mistyrose2"] = QColor(238, 213, 210);
	m_colorMap["mistyrose3"] = QColor(205, 183, 181);
	m_colorMap["mistyrose4"] = QColor(139, 125, 123);
	m_colorMap["azure1"] = QColor(240, 255, 255);
	m_colorMap["azure2"] = QColor(224, 238, 238);
	m_colorMap["azure3"] = QColor(193, 205, 205);
	m_colorMap["azure4"] = QColor(131, 139, 139);
	m_colorMap["slateblue1"] = QColor(131, 111, 255);
	m_colorMap["slateblue2"] = QColor(122, 103, 238);
	m_colorMap["slateblue3"] = QColor(105, 89, 205);
	m_colorMap["slateblue4"] = QColor(71, 60, 139);
	m_colorMap["royalblue1"] = QColor(72, 118, 255);
	m_colorMap["royalblue2"] = QColor(67, 110, 238);
	m_colorMap["royalblue3"] = QColor(58, 95, 205);
	m_colorMap["royalblue4"] = QColor(39, 64, 139);
	m_colorMap["blue1"] = QColor(0, 0, 255);
	m_colorMap["blue2"] = QColor(0, 0, 238);
	m_colorMap["blue3"] = QColor(0, 0, 205);
	m_colorMap["blue4"] = QColor(0, 0, 139);
	m_colorMap["dodgerblue1"] = QColor(30, 144, 255);
	m_colorMap["dodgerblue2"] = QColor(28, 134, 238);
	m_colorMap["dodgerblue3"] = QColor(24, 116, 205);
	m_colorMap["dodgerblue4"] = QColor(16, 78, 139);
	m_colorMap["steelblue1"] = QColor(99, 184, 255);
	m_colorMap["steelblue2"] = QColor(92, 172, 238);
	m_colorMap["steelblue3"] = QColor(79, 148, 205);
	m_colorMap["steelblue4"] = QColor(54, 100, 139);
	m_colorMap["deepskyblue1"] = QColor(0, 191, 255);
	m_colorMap["deepskyblue2"] = QColor(0, 178, 238);
	m_colorMap["deepskyblue3"] = QColor(0, 154, 205);
	m_colorMap["deepskyblue4"] = QColor(0, 104, 139);
	m_colorMap["skyblue1"] = QColor(135, 206, 255);
	m_colorMap["skyblue2"] = QColor(126, 192, 238);
	m_colorMap["skyblue3"] = QColor(108, 166, 205);
	m_colorMap["skyblue4"] = QColor(74, 112, 139);
	m_colorMap["lightskyblue1"] = QColor(176, 226, 255);
	m_colorMap["lightskyblue2"] = QColor(164, 211, 238);
	m_colorMap["lightskyblue3"] = QColor(141, 182, 205);
	m_colorMap["lightskyblue4"] = QColor(96, 123, 139);
	m_colorMap["slategray1"] = QColor(198, 226, 255);
	m_colorMap["slategray2"] = QColor(185, 211, 238);
	m_colorMap["slategray3"] = QColor(159, 182, 205);
	m_colorMap["slategray4"] = QColor(108, 123, 139);
	m_colorMap["lightsteelblue1"] = QColor(202, 225, 255);
	m_colorMap["lightsteelblue2"] = QColor(188, 210, 238);
	m_colorMap["lightsteelblue3"] = QColor(162, 181, 205);
	m_colorMap["lightsteelblue4"] = QColor(110, 123, 139);
	m_colorMap["lightblue1"] = QColor(191, 239, 255);
	m_colorMap["lightblue2"] = QColor(178, 223, 238);
	m_colorMap["lightblue3"] = QColor(154, 192, 205);
	m_colorMap["lightblue4"] = QColor(104, 131, 139);
	m_colorMap["lightcyan1"] = QColor(224, 255, 255);
	m_colorMap["lightcyan2"] = QColor(209, 238, 238);
	m_colorMap["lightcyan3"] = QColor(180, 205, 205);
	m_colorMap["lightcyan4"] = QColor(122, 139, 139);
	m_colorMap["paleturquoise1"] = QColor(187, 255, 255);
	m_colorMap["paleturquoise2"] = QColor(174, 238, 238);
	m_colorMap["paleturquoise3"] = QColor(150, 205, 205);
	m_colorMap["paleturquoise4"] = QColor(102, 139, 139);
	m_colorMap["cadetblue1"] = QColor(152, 245, 255);
	m_colorMap["cadetblue2"] = QColor(142, 229, 238);
	m_colorMap["cadetblue3"] = QColor(122, 197, 205);
	m_colorMap["cadetblue4"] = QColor(83, 134, 139);
	m_colorMap["turquoise1"] = QColor(0, 245, 255);
	m_colorMap["turquoise2"] = QColor(0, 229, 238);
	m_colorMap["turquoise3"] = QColor(0, 197, 205);
	m_colorMap["turquoise4"] = QColor(0, 134, 139);
	m_colorMap["cyan1"] = QColor(0, 255, 255);
	m_colorMap["cyan2"] = QColor(0, 238, 238);
	m_colorMap["cyan3"] = QColor(0, 205, 205);
	m_colorMap["cyan4"] = QColor(0, 139, 139);
	m_colorMap["darkslategray1"] = QColor(151, 255, 255);
	m_colorMap["darkslategray2"] = QColor(141, 238, 238);
	m_colorMap["darkslategray3"] = QColor(121, 205, 205);
	m_colorMap["darkslategray4"] = QColor(82, 139, 139);
	m_colorMap["aquamarine1"] = QColor(127, 255, 212);
	m_colorMap["aquamarine2"] = QColor(118, 238, 198);
	m_colorMap["aquamarine3"] = QColor(102, 205, 170);
	m_colorMap["aquamarine4"] = QColor(69, 139, 116);
	m_colorMap["darkseagreen1"] = QColor(193, 255, 193);
	m_colorMap["darkseagreen2"] = QColor(180, 238, 180);
	m_colorMap["darkseagreen3"] = QColor(155, 205, 155);
	m_colorMap["darkseagreen4"] = QColor(105, 139, 105);
	m_colorMap["seagreen1"] = QColor(84, 255, 159);
	m_colorMap["seagreen2"] = QColor(78, 238, 148);
	m_colorMap["seagreen3"] = QColor(67, 205, 128);
	m_colorMap["seagreen4"] = QColor(46, 139, 87);
	m_colorMap["palegreen1"] = QColor(154, 255, 154);
	m_colorMap["palegreen2"] = QColor(144, 238, 144);
	m_colorMap["palegreen3"] = QColor(124, 205, 124);
	m_colorMap["palegreen4"] = QColor(84, 139, 84);
	m_colorMap["springgreen1"] = QColor(0, 255, 127);
	m_colorMap["springgreen2"] = QColor(0, 238, 118);
	m_colorMap["springgreen3"] = QColor(0, 205, 102);
	m_colorMap["springgreen4"] = QColor(0, 139, 69);
	m_colorMap["green1"] = QColor(0, 255, 0);
	m_colorMap["green2"] = QColor(0, 238, 0);
	m_colorMap["green3"] = QColor(0, 205, 0);
	m_colorMap["green4"] = QColor(0, 139, 0);
	m_colorMap["chartreuse1"] = QColor(127, 255, 0);
	m_colorMap["chartreuse2"] = QColor(118, 238, 0);
	m_colorMap["chartreuse3"] = QColor(102, 205, 0);
	m_colorMap["chartreuse4"] = QColor(69, 139, 0);
	m_colorMap["olivedrab1"] = QColor(192, 255, 62);
	m_colorMap["olivedrab2"] = QColor(179, 238, 58);
	m_colorMap["olivedrab3"] = QColor(154, 205, 50);
	m_colorMap["olivedrab4"] = QColor(105, 139, 34);
	m_colorMap["darkolivegreen1"] = QColor(202, 255, 112);
	m_colorMap["darkolivegreen2"] = QColor(188, 238, 104);
	m_colorMap["darkolivegreen3"] = QColor(162, 205, 90);
	m_colorMap["darkolivegreen4"] = QColor(110, 139, 61);
	m_colorMap["khaki1"] = QColor(255, 246, 143);
	m_colorMap["khaki2"] = QColor(238, 230, 133);
	m_colorMap["khaki3"] = QColor(205, 198, 115);
	m_colorMap["khaki4"] = QColor(139, 134, 78);
	m_colorMap["lightgoldenrod1"] = QColor(255, 236, 139);
	m_colorMap["lightgoldenrod2"] = QColor(238, 220, 130);
	m_colorMap["lightgoldenrod3"] = QColor(205, 190, 112);
	m_colorMap["lightgoldenrod4"] = QColor(139, 129, 76);
	m_colorMap["lightyellow1"] = QColor(255, 255, 224);
	m_colorMap["lightyellow2"] = QColor(238, 238, 209);
	m_colorMap["lightyellow3"] = QColor(205, 205, 180);
	m_colorMap["lightyellow4"] = QColor(139, 139, 122);
	m_colorMap["yellow1"] = QColor(255, 255, 0);
	m_colorMap["yellow2"] = QColor(238, 238, 0);
	m_colorMap["yellow3"] = QColor(205, 205, 0);
	m_colorMap["yellow4"] = QColor(139, 139, 0);
	m_colorMap["gold1"] = QColor(255, 215, 0);
	m_colorMap["gold2"] = QColor(238, 201, 0);
	m_colorMap["gold3"] = QColor(205, 173, 0);
	m_colorMap["gold4"] = QColor(139, 117, 0);
	m_colorMap["goldenrod1"] = QColor(255, 193, 37);
	m_colorMap["goldenrod2"] = QColor(238, 180, 34);
	m_colorMap["goldenrod3"] = QColor(205, 155, 29);
	m_colorMap["goldenrod4"] = QColor(139, 105, 20);
	m_colorMap["darkgoldenrod1"] = QColor(255, 185, 15);
	m_colorMap["darkgoldenrod2"] = QColor(238, 173, 14);
	m_colorMap["darkgoldenrod3"] = QColor(205, 149, 12);
	m_colorMap["darkgoldenrod4"] = QColor(139, 101, 8);
	m_colorMap["rosybrown1"] = QColor(255, 193, 193);
	m_colorMap["rosybrown2"] = QColor(238, 180, 180);
	m_colorMap["rosybrown3"] = QColor(205, 155, 155);
	m_colorMap["rosybrown4"] = QColor(139, 105, 105);
	m_colorMap["indianred1"] = QColor(255, 106, 106);
	m_colorMap["indianred2"] = QColor(238, 99, 99);
	m_colorMap["indianred3"] = QColor(205, 85, 85);
	m_colorMap["indianred4"] = QColor(139, 58, 58);
	m_colorMap["sienna1"] = QColor(255, 130, 71);
	m_colorMap["sienna2"] = QColor(238, 121, 66);
	m_colorMap["sienna3"] = QColor(205, 104, 57);
	m_colorMap["sienna4"] = QColor(139, 71, 38);
	m_colorMap["burlywood1"] = QColor(255, 211, 155);
	m_colorMap["burlywood2"] = QColor(238, 197, 145);
	m_colorMap["burlywood3"] = QColor(205, 170, 125);
	m_colorMap["burlywood4"] = QColor(139, 115, 85);
	m_colorMap["wheat1"] = QColor(255, 231, 186);
	m_colorMap["wheat2"] = QColor(238, 216, 174);
	m_colorMap["wheat3"] = QColor(205, 186, 150);
	m_colorMap["wheat4"] = QColor(139, 126, 102);
	m_colorMap["tan1"] = QColor(255, 165, 79);
	m_colorMap["tan2"] = QColor(238, 154, 73);
	m_colorMap["tan3"] = QColor(205, 133, 63);
	m_colorMap["tan4"] = QColor(139, 90, 43);
	m_colorMap["chocolate1"] = QColor(255, 127, 36);
	m_colorMap["chocolate2"] = QColor(238, 118, 33);
	m_colorMap["chocolate3"] = QColor(205, 102, 29);
	m_colorMap["chocolate4"] = QColor(139, 69, 19);
	m_colorMap["firebrick1"] = QColor(255, 48, 48);
	m_colorMap["firebrick2"] = QColor(238, 44, 44);
	m_colorMap["firebrick3"] = QColor(205, 38, 38);
	m_colorMap["firebrick4"] = QColor(139, 26, 26);
	m_colorMap["brown1"] = QColor(255, 64, 64);
	m_colorMap["brown2"] = QColor(238, 59, 59);
	m_colorMap["brown3"] = QColor(205, 51, 51);
	m_colorMap["brown4"] = QColor(139, 35, 35);
	m_colorMap["salmon1"] = QColor(255, 140, 105);
	m_colorMap["salmon2"] = QColor(238, 130, 98);
	m_colorMap["salmon3"] = QColor(205, 112, 84);
	m_colorMap["salmon4"] = QColor(139, 76, 57);
	m_colorMap["lightsalmon1"] = QColor(255, 160, 122);
	m_colorMap["lightsalmon2"] = QColor(238, 149, 114);
	m_colorMap["lightsalmon3"] = QColor(205, 129, 98);
	m_colorMap["lightsalmon4"] = QColor(139, 87, 66);
	m_colorMap["orange1"] = QColor(255, 165, 0);
	m_colorMap["orange2"] = QColor(238, 154, 0);
	m_colorMap["orange3"] = QColor(205, 133, 0);
	m_colorMap["orange4"] = QColor(139, 90, 0);
	m_colorMap["darkorange1"] = QColor(255, 127, 0);
	m_colorMap["darkorange2"] = QColor(238, 118, 0);
	m_colorMap["darkorange3"] = QColor(205, 102, 0);
	m_colorMap["darkorange4"] = QColor(139, 69, 0);
	m_colorMap["coral1"] = QColor(255, 114, 86);
	m_colorMap["coral2"] = QColor(238, 106, 80);
	m_colorMap["coral3"] = QColor(205, 91, 69);
	m_colorMap["coral4"] = QColor(139, 62, 47);
	m_colorMap["tomato1"] = QColor(255, 99, 71);
	m_colorMap["tomato2"] = QColor(238, 92, 66);
	m_colorMap["tomato3"] = QColor(205, 79, 57);
	m_colorMap["tomato4"] = QColor(139, 54, 38);
	m_colorMap["orangered1"] = QColor(255, 69, 0);
	m_colorMap["orangered2"] = QColor(238, 64, 0);
	m_colorMap["orangered3"] = QColor(205, 55, 0);
	m_colorMap["orangered4"] = QColor(139, 37, 0);
	m_colorMap["red1"] = QColor(255, 0, 0);
	m_colorMap["red2"] = QColor(238, 0, 0);
	m_colorMap["red3"] = QColor(205, 0, 0);
	m_colorMap["red4"] = QColor(139, 0, 0);
	m_colorMap["deeppink1"] = QColor(255, 20, 147);
	m_colorMap["deeppink2"] = QColor(238, 18, 137);
	m_colorMap["deeppink3"] = QColor(205, 16, 118);
	m_colorMap["deeppink4"] = QColor(139, 10, 80);
	m_colorMap["hotpink1"] = QColor(255, 110, 180);
	m_colorMap["hotpink2"] = QColor(238, 106, 167);
	m_colorMap["hotpink3"] = QColor(205, 96, 144);
	m_colorMap["hotpink4"] = QColor(139, 58, 98);
	m_colorMap["pink1"] = QColor(255, 181, 197);
	m_colorMap["pink2"] = QColor(238, 169, 184);
	m_colorMap["pink3"] = QColor(205, 145, 158);
	m_colorMap["pink4"] = QColor(139, 99, 108);
	m_colorMap["lightpink1"] = QColor(255, 174, 185);
	m_colorMap["lightpink2"] = QColor(238, 162, 173);
	m_colorMap["lightpink3"] = QColor(205, 140, 149);
	m_colorMap["lightpink4"] = QColor(139, 95, 101);
	m_colorMap["palevioletred1"] = QColor(255, 130, 171);
	m_colorMap["palevioletred2"] = QColor(238, 121, 159);
	m_colorMap["palevioletred3"] = QColor(205, 104, 137);
	m_colorMap["palevioletred4"] = QColor(139, 71, 93);
	m_colorMap["maroon1"] = QColor(255, 52, 179);
	m_colorMap["maroon2"] = QColor(238, 48, 167);
	m_colorMap["maroon3"] = QColor(205, 41, 144);
	m_colorMap["maroon4"] = QColor(139, 28, 98);
	m_colorMap["violetred1"] = QColor(255, 62, 150);
	m_colorMap["violetred2"] = QColor(238, 58, 140);
	m_colorMap["violetred3"] = QColor(205, 50, 120);
	m_colorMap["violetred4"] = QColor(139, 34, 82);
	m_colorMap["magenta1"] = QColor(255, 0, 255);
	m_colorMap["magenta2"] = QColor(238, 0, 238);
	m_colorMap["magenta3"] = QColor(205, 0, 205);
	m_colorMap["magenta4"] = QColor(139, 0, 139);
	m_colorMap["orchid1"] = QColor(255, 131, 250);
	m_colorMap["orchid2"] = QColor(238, 122, 233);
	m_colorMap["orchid3"] = QColor(205, 105, 201);
	m_colorMap["orchid4"] = QColor(139, 71, 137);
	m_colorMap["plum1"] = QColor(255, 187, 255);
	m_colorMap["plum2"] = QColor(238, 174, 238);
	m_colorMap["plum3"] = QColor(205, 150, 205);
	m_colorMap["plum4"] = QColor(139, 102, 139);
	m_colorMap["mediumorchid1"] = QColor(224, 102, 255);
	m_colorMap["mediumorchid2"] = QColor(209, 95, 238);
	m_colorMap["mediumorchid3"] = QColor(180, 82, 205);
	m_colorMap["mediumorchid4"] = QColor(122, 55, 139);
	m_colorMap["darkorchid1"] = QColor(191, 62, 255);
	m_colorMap["darkorchid2"] = QColor(178, 58, 238);
	m_colorMap["darkorchid3"] = QColor(154, 50, 205);
	m_colorMap["darkorchid4"] = QColor(104, 34, 139);
	m_colorMap["purple1"] = QColor(155, 48, 255);
	m_colorMap["purple2"] = QColor(145, 44, 238);
	m_colorMap["purple3"] = QColor(125, 38, 205);
	m_colorMap["purple4"] = QColor(85, 26, 139);
	m_colorMap["mediumpurple1"] = QColor(171, 130, 255);
	m_colorMap["mediumpurple2"] = QColor(159, 121, 238);
	m_colorMap["mediumpurple3"] = QColor(137, 104, 205);
	m_colorMap["mediumpurple4"] = QColor(93, 71, 139);
	m_colorMap["thistle1"] = QColor(255, 225, 255);
	m_colorMap["thistle2"] = QColor(238, 210, 238);
	m_colorMap["thistle3"] = QColor(205, 181, 205);
	m_colorMap["thistle4"] = QColor(139, 123, 139);
	m_colorMap["gray0"] = QColor(0, 0, 0);
	m_colorMap["grey0"] = QColor(0, 0, 0);
	m_colorMap["gray1"] = QColor(3, 3, 3);
	m_colorMap["grey1"] = QColor(3, 3, 3);
	m_colorMap["gray2"] = QColor(5, 5, 5);
	m_colorMap["grey2"] = QColor(5, 5, 5);
	m_colorMap["gray3"] = QColor(8, 8, 8);
	m_colorMap["grey3"] = QColor(8, 8, 8);
	m_colorMap["gray4"] = QColor(10, 10, 10);
	m_colorMap["grey4"] = QColor(10, 10, 10);
	m_colorMap["gray5"] = QColor(13, 13, 13);
	m_colorMap["grey5"] = QColor(13, 13, 13);
	m_colorMap["gray6"] = QColor(15, 15, 15);
	m_colorMap["grey6"] = QColor(15, 15, 15);
	m_colorMap["gray7"] = QColor(18, 18, 18);
	m_colorMap["grey7"] = QColor(18, 18, 18);
	m_colorMap["gray8"] = QColor(20, 20, 20);
	m_colorMap["grey8"] = QColor(20, 20, 20);
	m_colorMap["gray9"] = QColor(23, 23, 23);
	m_colorMap["grey9"] = QColor(23, 23, 23);
	m_colorMap["gray10"] = QColor(26, 26, 26);
	m_colorMap["grey10"] = QColor(26, 26, 26);
	m_colorMap["gray11"] = QColor(28, 28, 28);
	m_colorMap["grey11"] = QColor(28, 28, 28);
	m_colorMap["gray12"] = QColor(31, 31, 31);
	m_colorMap["grey12"] = QColor(31, 31, 31);
	m_colorMap["gray13"] = QColor(33, 33, 33);
	m_colorMap["grey13"] = QColor(33, 33, 33);
	m_colorMap["gray14"] = QColor(36, 36, 36);
	m_colorMap["grey14"] = QColor(36, 36, 36);
	m_colorMap["gray15"] = QColor(38, 38, 38);
	m_colorMap["grey15"] = QColor(38, 38, 38);
	m_colorMap["gray16"] = QColor(41, 41, 41);
	m_colorMap["grey16"] = QColor(41, 41, 41);
	m_colorMap["gray17"] = QColor(43, 43, 43);
	m_colorMap["grey17"] = QColor(43, 43, 43);
	m_colorMap["gray18"] = QColor(46, 46, 46);
	m_colorMap["grey18"] = QColor(46, 46, 46);
	m_colorMap["gray19"] = QColor(48, 48, 48);
	m_colorMap["grey19"] = QColor(48, 48, 48);
	m_colorMap["gray20"] = QColor(51, 51, 51);
	m_colorMap["grey20"] = QColor(51, 51, 51);
	m_colorMap["gray21"] = QColor(54, 54, 54);
	m_colorMap["grey21"] = QColor(54, 54, 54);
	m_colorMap["gray22"] = QColor(56, 56, 56);
	m_colorMap["grey22"] = QColor(56, 56, 56);
	m_colorMap["gray23"] = QColor(59, 59, 59);
	m_colorMap["grey23"] = QColor(59, 59, 59);
	m_colorMap["gray24"] = QColor(61, 61, 61);
	m_colorMap["grey24"] = QColor(61, 61, 61);
	m_colorMap["gray25"] = QColor(64, 64, 64);
	m_colorMap["grey25"] = QColor(64, 64, 64);
	m_colorMap["gray26"] = QColor(66, 66, 66);
	m_colorMap["grey26"] = QColor(66, 66, 66);
	m_colorMap["gray27"] = QColor(69, 69, 69);
	m_colorMap["grey27"] = QColor(69, 69, 69);
	m_colorMap["gray28"] = QColor(71, 71, 71);
	m_colorMap["grey28"] = QColor(71, 71, 71);
	m_colorMap["gray29"] = QColor(74, 74, 74);
	m_colorMap["grey29"] = QColor(74, 74, 74);
	m_colorMap["gray30"] = QColor(77, 77, 77);
	m_colorMap["grey30"] = QColor(77, 77, 77);
	m_colorMap["gray31"] = QColor(79, 79, 79);
	m_colorMap["grey31"] = QColor(79, 79, 79);
	m_colorMap["gray32"] = QColor(82, 82, 82);
	m_colorMap["grey32"] = QColor(82, 82, 82);
	m_colorMap["gray33"] = QColor(84, 84, 84);
	m_colorMap["grey33"] = QColor(84, 84, 84);
	m_colorMap["gray34"] = QColor(87, 87, 87);
	m_colorMap["grey34"] = QColor(87, 87, 87);
	m_colorMap["gray35"] = QColor(89, 89, 89);
	m_colorMap["grey35"] = QColor(89, 89, 89);
	m_colorMap["gray36"] = QColor(92, 92, 92);
	m_colorMap["grey36"] = QColor(92, 92, 92);
	m_colorMap["gray37"] = QColor(94, 94, 94);
	m_colorMap["grey37"] = QColor(94, 94, 94);
	m_colorMap["gray38"] = QColor(97, 97, 97);
	m_colorMap["grey38"] = QColor(97, 97, 97);
	m_colorMap["gray39"] = QColor(99, 99, 99);
	m_colorMap["grey39"] = QColor(99, 99, 99);
	m_colorMap["gray40"] = QColor(102, 102, 102);
	m_colorMap["grey40"] = QColor(102, 102, 102);
	m_colorMap["gray41"] = QColor(105, 105, 105);
	m_colorMap["grey41"] = QColor(105, 105, 105);
	m_colorMap["gray42"] = QColor(107, 107, 107);
	m_colorMap["grey42"] = QColor(107, 107, 107);
	m_colorMap["gray43"] = QColor(110, 110, 110);
	m_colorMap["grey43"] = QColor(110, 110, 110);
	m_colorMap["gray44"] = QColor(112, 112, 112);
	m_colorMap["grey44"] = QColor(112, 112, 112);
	m_colorMap["gray45"] = QColor(115, 115, 115);
	m_colorMap["grey45"] = QColor(115, 115, 115);
	m_colorMap["gray46"] = QColor(117, 117, 117);
	m_colorMap["grey46"] = QColor(117, 117, 117);
	m_colorMap["gray47"] = QColor(120, 120, 120);
	m_colorMap["grey47"] = QColor(120, 120, 120);
	m_colorMap["gray48"] = QColor(122, 122, 122);
	m_colorMap["grey48"] = QColor(122, 122, 122);
	m_colorMap["gray49"] = QColor(125, 125, 125);
	m_colorMap["grey49"] = QColor(125, 125, 125);
	m_colorMap["gray50"] = QColor(127, 127, 127);
	m_colorMap["grey50"] = QColor(127, 127, 127);
	m_colorMap["gray51"] = QColor(130, 130, 130);
	m_colorMap["grey51"] = QColor(130, 130, 130);
	m_colorMap["gray52"] = QColor(133, 133, 133);
	m_colorMap["grey52"] = QColor(133, 133, 133);
	m_colorMap["gray53"] = QColor(135, 135, 135);
	m_colorMap["grey53"] = QColor(135, 135, 135);
	m_colorMap["gray54"] = QColor(138, 138, 138);
	m_colorMap["grey54"] = QColor(138, 138, 138);
	m_colorMap["gray55"] = QColor(140, 140, 140);
	m_colorMap["grey55"] = QColor(140, 140, 140);
	m_colorMap["gray56"] = QColor(143, 143, 143);
	m_colorMap["grey56"] = QColor(143, 143, 143);
	m_colorMap["gray57"] = QColor(145, 145, 145);
	m_colorMap["grey57"] = QColor(145, 145, 145);
	m_colorMap["gray58"] = QColor(148, 148, 148);
	m_colorMap["grey58"] = QColor(148, 148, 148);
	m_colorMap["gray59"] = QColor(150, 150, 150);
	m_colorMap["grey59"] = QColor(150, 150, 150);
	m_colorMap["gray60"] = QColor(153, 153, 153);
	m_colorMap["grey60"] = QColor(153, 153, 153);
	m_colorMap["gray61"] = QColor(156, 156, 156);
	m_colorMap["grey61"] = QColor(156, 156, 156);
	m_colorMap["gray62"] = QColor(158, 158, 158);
	m_colorMap["grey62"] = QColor(158, 158, 158);
	m_colorMap["gray63"] = QColor(161, 161, 161);
	m_colorMap["grey63"] = QColor(161, 161, 161);
	m_colorMap["gray64"] = QColor(163, 163, 163);
	m_colorMap["grey64"] = QColor(163, 163, 163);
	m_colorMap["gray65"] = QColor(166, 166, 166);
	m_colorMap["grey65"] = QColor(166, 166, 166);
	m_colorMap["gray66"] = QColor(168, 168, 168);
	m_colorMap["grey66"] = QColor(168, 168, 168);
	m_colorMap["gray67"] = QColor(171, 171, 171);
	m_colorMap["grey67"] = QColor(171, 171, 171);
	m_colorMap["gray68"] = QColor(173, 173, 173);
	m_colorMap["grey68"] = QColor(173, 173, 173);
	m_colorMap["gray69"] = QColor(176, 176, 176);
	m_colorMap["grey69"] = QColor(176, 176, 176);
	m_colorMap["gray70"] = QColor(179, 179, 179);
	m_colorMap["grey70"] = QColor(179, 179, 179);
	m_colorMap["gray71"] = QColor(181, 181, 181);
	m_colorMap["grey71"] = QColor(181, 181, 181);
	m_colorMap["gray72"] = QColor(184, 184, 184);
	m_colorMap["grey72"] = QColor(184, 184, 184);
	m_colorMap["gray73"] = QColor(186, 186, 186);
	m_colorMap["grey73"] = QColor(186, 186, 186);
	m_colorMap["gray74"] = QColor(189, 189, 189);
	m_colorMap["grey74"] = QColor(189, 189, 189);
	m_colorMap["gray75"] = QColor(191, 191, 191);
	m_colorMap["grey75"] = QColor(191, 191, 191);
	m_colorMap["gray76"] = QColor(194, 194, 194);
	m_colorMap["grey76"] = QColor(194, 194, 194);
	m_colorMap["gray77"] = QColor(196, 196, 196);
	m_colorMap["grey77"] = QColor(196, 196, 196);
	m_colorMap["gray78"] = QColor(199, 199, 199);
	m_colorMap["grey78"] = QColor(199, 199, 199);
	m_colorMap["gray79"] = QColor(201, 201, 201);
	m_colorMap["grey79"] = QColor(201, 201, 201);
	m_colorMap["gray80"] = QColor(204, 204, 204);
	m_colorMap["grey80"] = QColor(204, 204, 204);
	m_colorMap["gray81"] = QColor(207, 207, 207);
	m_colorMap["grey81"] = QColor(207, 207, 207);
	m_colorMap["gray82"] = QColor(209, 209, 209);
	m_colorMap["grey82"] = QColor(209, 209, 209);
	m_colorMap["gray83"] = QColor(212, 212, 212);
	m_colorMap["grey83"] = QColor(212, 212, 212);
	m_colorMap["gray84"] = QColor(214, 214, 214);
	m_colorMap["grey84"] = QColor(214, 214, 214);
	m_colorMap["gray85"] = QColor(217, 217, 217);
	m_colorMap["grey85"] = QColor(217, 217, 217);
	m_colorMap["gray86"] = QColor(219, 219, 219);
	m_colorMap["grey86"] = QColor(219, 219, 219);
	m_colorMap["gray87"] = QColor(222, 222, 222);
	m_colorMap["grey87"] = QColor(222, 222, 222);
	m_colorMap["gray88"] = QColor(224, 224, 224);
	m_colorMap["grey88"] = QColor(224, 224, 224);
	m_colorMap["gray89"] = QColor(227, 227, 227);
	m_colorMap["grey89"] = QColor(227, 227, 227);
	m_colorMap["gray90"] = QColor(229, 229, 229);
	m_colorMap["grey90"] = QColor(229, 229, 229);
	m_colorMap["gray91"] = QColor(232, 232, 232);
	m_colorMap["grey91"] = QColor(232, 232, 232);
	m_colorMap["gray92"] = QColor(235, 235, 235);
	m_colorMap["grey92"] = QColor(235, 235, 235);
	m_colorMap["gray93"] = QColor(237, 237, 237);
	m_colorMap["grey93"] = QColor(237, 237, 237);
	m_colorMap["gray94"] = QColor(240, 240, 240);
	m_colorMap["grey94"] = QColor(240, 240, 240);
	m_colorMap["gray95"] = QColor(242, 242, 242);
	m_colorMap["grey95"] = QColor(242, 242, 242);
	m_colorMap["gray96"] = QColor(245, 245, 245);
	m_colorMap["grey96"] = QColor(245, 245, 245);
	m_colorMap["gray97"] = QColor(247, 247, 247);
	m_colorMap["grey97"] = QColor(247, 247, 247);
	m_colorMap["gray98"] = QColor(250, 250, 250);
	m_colorMap["grey98"] = QColor(250, 250, 250);
	m_colorMap["gray99"] = QColor(252, 252, 252);
	m_colorMap["grey99"] = QColor(252, 252, 252);
	m_colorMap["gray100"] = QColor(255, 255, 255);
	m_colorMap["grey100"] = QColor(255, 255, 255);
	m_colorMap["darkgrey"] = QColor(169, 169, 169);
	m_colorMap["darkgray"] = QColor(169, 169, 169);
	m_colorMap["darkblue"] = QColor(0, 0, 139);
	m_colorMap["darkcyan"] = QColor(0, 139, 139);
	m_colorMap["darkmagenta"] = QColor(139, 0, 139);
	m_colorMap["darkred"] = QColor(139, 0, 0);
	m_colorMap["lightgreen"] = QColor(144, 238, 144);
}
