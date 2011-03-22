#include "qvimshell.moc"

extern "C" {
#include "proto/gui.pro"
}


QVimShell::QVimShell(gui_T *gui, QWidget *parent)
:QWidget(parent), m_foreground(Qt::black), m_gui(gui), 
	m_input(false)
{
	setAttribute(Qt::WA_KeyCompression, true);
	setMouseTracking(true);
}

void QVimShell::setBackground(const QColor& color)
{
	m_background = color;
}
QColor QVimShell::background()
{
	return m_background;
}

void QVimShell::setForeground(const QColor& color)
{
	m_foreground = color;
}

QColor QVimShell::foreground()
{
	return m_foreground;
}


void QVimShell::setSpecial(const QColor& color)
{
	m_special = QBrush(color);
}

void QVimShell::resizeEvent(QResizeEvent *ev)
{
	canvas = QPixmap( ev->size() );
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

bool QVimShell::hasInput()
{
	if (m_input) {
		m_input = false;
		return true;
	}
	return false;
}

void QVimShell::forceInput()
{
	m_input = true;
}

void QVimShell::keyPressEvent ( QKeyEvent *ev)
{
	char str[3];

	m_input = true;
	if ( specialKey( ev->key(), str)) {
		add_to_input_buf((char_u *) str, 3);
		return;
	}

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

void QVimShell::flushPaintOps()
{
	QPainter painter(&canvas);
	while ( !paintOps.isEmpty() ) {

		PaintOperation op = paintOps.dequeue();
		switch( op.type ) {
		case CLEARALL:
			painter.fillRect(canvas.rect(), op.color);
			break;
		case FILLRECT:
			painter.fillRect(op.rect, op.color);
			break;
		case DRAWRECT:
			painter.drawRect(op.rect); // FIXME: need color
			break;
		case DRAWSTRING:
			painter.setPen( op.color );
			painter.setFont( op.font );
			painter.drawText(op.rect, op.str);
			break;
		case INVERTRECT:
			painter.setCompositionMode( QPainter::CompositionMode_Difference );
			painter.fillRect( op.rect, Qt::black);
			painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
			break;
		case SCROLLRECT:
			painter.end();

			QRegion exposed;
			canvas.scroll(op.pos.x(), op.pos.y(),
			 	op.rect, &exposed);

			painter.begin(&canvas);
			painter.fillRect(exposed.boundingRect(), op.color);
			break;
		}
	}
}


void QVimShell::paintEvent ( QPaintEvent *ev )
{
	flushPaintOps();
	QPainter realpainter(this);
	realpainter.drawPixmap( ev->rect(), canvas, ev->rect());
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
	m_input = true;
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
	m_input = true;
}

void QVimShell::mouseDoubleClickEvent(QMouseEvent *ev)
{
	gui_send_mouse_event(MOUSE_LEFT, ev->pos().x(),
					  ev->pos().y(), TRUE, 0);
	m_input = true;
}

void QVimShell::mouseReleaseEvent(QMouseEvent *ev)
{
	gui_send_mouse_event(MOUSE_RELEASE, ev->pos().x(),
					  ev->pos().y(), FALSE, 0);
	m_input = true;
}

void QVimShell::wheelEvent(QWheelEvent *ev)
{
	gui_send_mouse_event((ev->delta() > 0) ? MOUSE_4 : MOUSE_5,
					    ev->pos().x(), ev->pos().y(), FALSE, 0);
	m_input = true;
}

QIcon QVimShell::iconFromTheme(const QString& name)
{
	QIcon icon;

	// Theme icons
	if ( "Open" == name ) {
		icon = QIcon::fromTheme("document-open");
	} else if ( "Save" == name ) {
		icon = QIcon::fromTheme("document-save");
	} else if ( "SaveAll" == name ) {
		icon = QIcon::fromTheme("document-save-as");
	} else if ( "Print" == name ) {
		icon = QIcon::fromTheme("document-print");
	} else if ( "Undo" == name ) {
		icon = QIcon::fromTheme("edit-undo");
	} else if ( "Redo"== name ) {
		icon = QIcon::fromTheme("edit-redo");
	} else if ( "Cut" == name ) {
		icon = QIcon::fromTheme("edit-cut");
	} else if ( "Copy" == name ) {
		icon = QIcon::fromTheme("edit-copy");
	} else if ( "Paste" == name ) {
		icon = QIcon::fromTheme("edit-paste");
	} else if ( "Replace" == name ) {
		icon = QIcon::fromTheme("edit-find-replace");
	} else if ( "FindNext" == name ) {
		icon = QIcon::fromTheme("go-next");
	} else if ( "FindPrev" == name ) {
		icon = QIcon::fromTheme("go-previous");
	} else if ( "LoadSesn" == name ) {
		icon = QIcon::fromTheme("folder-open");
	} else if ( "SaveSesn" == name ) {
	} else if ( "RunScript" == name ) {
		icon = QIcon::fromTheme("system-run");
	} else if ( "Make" == name ) {
	} else if ( "RunCtags" == name ) {
	} else if ( "TagJump" == name ) {
		icon = QIcon::fromTheme("go-jump");
	} else if ( "FindHelp" == name ) {
		icon = QIcon::fromTheme("help-contents");
	}

	if ( icon.isNull() ) { // last resort
		icon = QIcon::fromTheme(name.toLower());
	}

	return icon;
}


/**
 * Get an icon for a given name
 *
 */
QIcon QVimShell::icon(const QString& name)
{
	QIcon icon = iconFromTheme(name);

	if ( icon.isNull() ) {
		return QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon);
	}

	return icon;
}

void QVimShell::loadColors(const QString& name)
{
	m_colorTable.clear();
	
	qDebug() << name;
	QFile f(name);
	if (!f.open(QFile::ReadOnly)) {
		return;
	}
	
	while (!f.atEnd()) {
		QString line = QString::fromUtf8( f.readLine() );
		if ( line.startsWith("!") ) {
			continue;
		}

		// FIXME: some color names have spaces
		QStringList list = line.split( " ", QString::SkipEmptyParts);
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

		QColor c(r,g,b);
		m_colorTable.insert(list[3].simplified(), c);
		qDebug() << list[3].simplified();
	}
}

QColor QVimShell::color(const QString& name)
{
	if ( QColor::isValidColor(name) ) {
		return QColor(name);
	}

	return m_colorTable.value( name, QColor());

}

void QVimShell::queuePaintOp(PaintOperation op)
{
	paintOps.enqueue(op);
	if ( op.rect.isValid() ) {
		update(op.rect); // FIXME: use rect
	} else {
		update();
	}
}
