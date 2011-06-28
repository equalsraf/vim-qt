#include "vimwrapper.moc"

extern "C" {
#include "vim.h"
}

Q_DECLARE_METATYPE( QList<QUrl> );

VimWrapper::VimWrapper(QObject *parent)
:QObject(parent)
{
	qRegisterMetaType<QList<QUrl> >("URLList");

}

void VimWrapper::guiResizeShell(int w, int h)
{
	QMetaObject::invokeMethod(this, 
				"slot_guiResizeShell", Qt::QueuedConnection, 
				Q_ARG(int, w), 
				Q_ARG(int, h)
			);
	
}
void VimWrapper::slot_guiResizeShell(int w, int h)
{
	gui_resize_shell(w,h);
}

void VimWrapper::guiShellClosed()
{
	QMetaObject::invokeMethod(this, 
				"slot_guiShellClosed", 
				Qt::QueuedConnection);
}
void VimWrapper::slot_guiShellClosed()
{
	gui_shell_closed();
}

void VimWrapper::guiSendMouseEvent(int button, int x, int y, int repeated_click, unsigned int modifiers)
{
	QMetaObject::invokeMethod(this, 
				"slot_guiSendMouseEvent", Qt::QueuedConnection, 
				Q_ARG(int, button), 
				Q_ARG(int, x), 
				Q_ARG(int, y), 
				Q_ARG(int, repeated_click), 
				Q_ARG(unsigned int, modifiers)
				);
}
void VimWrapper::slot_guiSendMouseEvent(int button, int x, int y, int repeated_click, unsigned int modifiers)
{
	gui_send_mouse_event(button, x, y, repeated_click, modifiers);
}

void VimWrapper::guiMouseMoved(int x, int y)
{
	QMetaObject::invokeMethod(this, 
				"slot_guiMouseMoved",  Qt::QueuedConnection, 
				Q_ARG(int, x),
				Q_ARG(int, y)
				);

}
void VimWrapper::slot_guiMouseMoved(int x, int y)
{
	gui_mouse_moved(x, y);
}

void VimWrapper::guiFocusChanged(int focus)
{
	QMetaObject::invokeMethod(this, 
				"slot_guiFocusChanged", 
				Qt::QueuedConnection, Q_ARG(int, focus));
}
void VimWrapper::slot_guiFocusChanged(int focus)
{
	gui_focus_change(focus);
}

void VimWrapper::sendTablineEvent(int ev)
{
	QMetaObject::invokeMethod(this, 
				"slot_sendTablineEvent", 
				Qt::QueuedConnection, Q_ARG(int, ev));
}
void VimWrapper::slot_sendTablineEvent(int ev)
{
	send_tabline_event(ev);
}

void VimWrapper::sendTablineMenuEvent(int idx, int ev)
{
	QMetaObject::invokeMethod(this, 
				"slot_sendTablineMenuEvent", 
				Qt::QueuedConnection, Q_ARG(int, ev),
							Q_ARG(int, ev));
}
void VimWrapper::slot_sendTablineMenuEvent(int idx, int ev)
{
	send_tabline_menu_event(idx, ev);
}


void VimWrapper::guiHandleDrop(int x, int y, unsigned int mod, const QList<QUrl> urls)
{
	QMetaObject::invokeMethod(this, 
				"slot_guiHandleDrop", 
				Qt::QueuedConnection, 
				Q_ARG(int, x),
				Q_ARG(int, y),
				Q_ARG(unsigned int, mod),
				Q_ARG(QList<QUrl>, urls)
				);
}
void VimWrapper::slot_guiHandleDrop(int x, int y, unsigned int mod, const QList<QUrl> urls)
{
	char_u **fnames = (char_u**)alloc( urls.size() * sizeof(char_u*));
	int i;
	for (i=0; i<urls.size(); i++) {
		QByteArray encoded = convertTo(urls.at(i).toString());
		char *s = (char*)alloc(encoded.size()*sizeof(char)+1);
		int j;
		for (j=0; j<encoded.size(); j++) {
			s[j] = encoded.at(j);
		}
		s[j]='\0';

		fnames[i] = (char_u *) s;
	}
	gui_handle_drop(x, y, 0, fnames, urls.size());
}


QPoint VimWrapper::mapText(int row, int col) 
{ 
	return QPoint( gui.char_width*col, gui.char_height*row );
}

QPoint VimWrapper::cursorPosition() 
{ 
	return mapText(gui.cursor_row, gui.cursor_col);
}

QRect VimWrapper::mapBlock(int row1, int col1, int row2, int col2)
{
	QPoint tl = mapText( row1, col1 );
	QPoint br = mapText( row2+1, col2+1);
	br.setX( br.x()-1 );
	br.setY( br.y()-1 );

	return QRect(tl, br);
}

QColor VimWrapper::backgroundColor()
{
	return fromColor(gui.back_pixel);
}

QColor VimWrapper::normalColor()
{
	return fromColor(gui.norm_pixel);
}

int VimWrapper::charWidth()
{
	return gui.char_width;
}

int VimWrapper::charHeight()
{
	return gui.char_height;
}

QFont VimWrapper::normalFont()
{
	if ( gui.norm_font ) {
		return *(gui.norm_font);
	}

	return QFont();
}

int VimWrapper::charCellWidth(const QChar& c)
{
	int len = utf_char2cells(c.unicode());
	if ( len <= 2 ) {
		return len;
	}

	return 0;
}

int VimWrapper::stringCellWidth(const QString& s)
{
	/*
	 * Vim kindly provides us with utf_char2cells,
	 * unfortunately Qt does not have a way measure
	 * wide char length.
	 */
	int len=0;
	foreach ( QChar c, s ) {
		len += charCellWidth(c);
	}
	return len;
}

QIcon VimWrapper::iconFromTheme(const QString& name)
{
	QIcon icon;

	// Theme icons
	if ( "Open" == name ) {
		icon = QIcon::fromTheme("document-open", 
				QIcon(":/icons/document-open.png"));
	} else if ( "Save" == name ) {
		icon = QIcon::fromTheme("document-save",
				QIcon(":/icons/document-save.png"));
	} else if ( "SaveAll" == name ) {
		icon = QIcon::fromTheme("document-save-all",
				QIcon(":/icons/document-save-all.png"));
	} else if ( "Print" == name ) {
		icon = QIcon::fromTheme("document-print",
				QIcon(":/icons/document-print.png"));
	} else if ( "Undo" == name ) {
		icon = QIcon::fromTheme("edit-undo",
				QIcon(":/icons/edit-undo.png"));
	} else if ( "Redo"== name ) {
		icon = QIcon::fromTheme("edit-redo",
				QIcon(":/icons/edit-redo.png"));
	} else if ( "Cut" == name ) {
		icon = QIcon::fromTheme("edit-cut",
				QIcon(":/icons/edit-cut.png"));
	} else if ( "Copy" == name ) {
		icon = QIcon::fromTheme("edit-copy",
				QIcon(":/icons/edit-copy.png"));
	} else if ( "Paste" == name ) {
		icon = QIcon::fromTheme("edit-paste",
				QIcon(":/icons/edit-paste.png"));
	} else if ( "Replace" == name ) {
		icon = QIcon::fromTheme("edit-find-replace",
				QIcon(":/icons/edit-find-replace.png"));
	} else if ( "FindNext" == name ) {
		icon = QIcon::fromTheme("go-next",
				QIcon(":/icons/go-next.png"));
	} else if ( "FindPrev" == name ) {
		icon = QIcon::fromTheme("go-previous",
				QIcon(":/icons/go-previous.png"));
	} else if ( "LoadSesn" == name ) {
		icon = QIcon::fromTheme("folder-open",
				QIcon(":/icons/document-open-folder.png"));
	} else if ( "SaveSesn" == name ) {
		icon = QIcon::fromTheme("document-save-as",
				QIcon(":/icons/document-save-as.png"));
	} else if ( "RunScript" == name ) {
		icon = QIcon::fromTheme("system-run",
				QIcon(":/icons/system-run.png"));
	} else if ( "Make" == name ) {
		icon = QIcon::fromTheme("run-build", 
				QIcon(":/icons/run-build.png"));
	} else if ( "RunCtags" == name ) {
		icon = QIcon(":/icons/table.png");
	} else if ( "TagJump" == name ) {
		icon = QIcon::fromTheme("go-jump",
				QIcon(":/icons/go-jump.png"));
	} else if ( "Help" == name ) {
		icon = QIcon::fromTheme("help-contents",
				QIcon(":/icons/help-contents.png"));
	} else if ( "FindHelp" == name ) {
		icon = QIcon::fromTheme("help-faq",
				QIcon(":/icons/help-contextual.png"));
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
QIcon VimWrapper::icon(const QString& name)
{
	QIcon icon = iconFromTheme(name);

	if ( icon.isNull() ) {
		return QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon);
	}

	return icon;
}


QColor
VimWrapper::fromColor(long color)
{
	if ( color == INVALCOLOR ) {
		return QColor();
	}

	int red = ((color & 0x00FF0000) >> 16);
	int green = ((color & 0x0000FF00) >>  8);
	int blue = ((color & 0x000000FF) >>  0);

	return QColor(red, green, blue);
}

long
VimWrapper::toColor(const QColor& c)
{
	return ((long)c.red() << 16) + ((long)c.green() << 8) + ((long)c.blue());
}

void
VimWrapper::newTab(int idx)
{
	send_tabline_menu_event(idx, TABLINE_MENU_NEW);
}

QByteArray VimWrapper::convertTo(const QString& s)
{
	bool m_encoding_utf8 = true; // FIXME: check encoding

	if ( m_encoding_utf8 ) {
		return s.toUtf8();
	} else {
		return s.toAscii();
	}
}

QString VimWrapper::convertFrom(const char *s, int size)
{
	bool m_encoding_utf8 = true; // FIXME: check encoding

	if ( m_encoding_utf8 ) {
		return QString::fromUtf8(s, size);
	} else {
		return QString::fromAscii(s, size);
	}
}


