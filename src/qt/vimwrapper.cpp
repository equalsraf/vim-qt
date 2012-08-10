#include "vimwrapper.moc"
#include <QApplication>
#include <QStyle>
#include <QMetaType>

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
	send_tabline_event(ev);
}

void VimWrapper::sendTablineMenuEvent(int idx, int ev)
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
	if ( urls.size() == 0 ) {
		return;
	}

	char_u **fnames = (char_u**)alloc( urls.size() * sizeof(char_u*));
	int i;
	for (i=0; i<urls.size(); i++) {
		QByteArray encoded;
		if ( urls.at(i).scheme() == "file" ) {
			encoded = convertTo(urls.at(i).toLocalFile());
		} else {
			encoded = convertTo(urls.at(i).toString());
		}

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
	br.setY( br.y() );

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

bool VimWrapper::isFakeMonospace(QFont f)
{

	QFont fi(f);
	fi.setItalic(true);
	QFont fb(f);
	fb.setBold(true);
	QFont fbi(fb);
	fbi.setItalic(false);

	QFontMetrics fm_normal(f);
	QFontMetrics fm_italic(fi);
	QFontMetrics fm_boldit(fbi);
	QFontMetrics fm_bold(fb);

	// Regular
	if ( fm_normal.averageCharWidth() != fm_normal.maxWidth() ) {
		QFontInfo info(f);
		qDebug() << __func__ << f.family() 
			<< "Average and Maximum font width mismatch for Regular font; QFont::exactMatch() is" << f.exactMatch()
			<< "Real font is " << info.family() << info.pointSize();
		return true;
	}

	// Italic
	if ( fm_italic.averageCharWidth() != fm_italic.maxWidth() ||
			fm_italic.maxWidth()*2 != fm_italic.width("MM") ) {
		QFontInfo info(fi);
		qDebug() << __func__ << fi.family() << "Average and Maximum font width mismatch for Italic font; QFont::exactMatch() is" << fi.exactMatch()
			<< "Real font is " << info.family() << info.pointSize();
		return true;
	}

	// Bold
	if ( fm_bold.averageCharWidth() != fm_bold.maxWidth() ||
			fm_bold.maxWidth()*2 != fm_bold.width("MM") ) {
		QFontInfo info(fb);
		qDebug() << __func__ << fb.family() << "Average and Maximum font width mismatch for Bold font; QFont::exactMatch() is" << fb.exactMatch()
			<< "Real font is " << info.family() << info.pointSize();
		return true;
	}

	// Bold+Italic
	if ( fm_boldit.averageCharWidth() != fm_boldit.maxWidth() ||
			fm_boldit.maxWidth()*2 != fm_boldit.width("MM") ) {
		QFontInfo info(fbi);
		qDebug() << __func__ << fbi.family() << "Average and Maximum font width mismatch for Bold+Italic font; QFont::exactMatch() is" << fbi.exactMatch()
			<< "Real font is " << info.family() << info.pointSize();
		return true;
	}

	if ( fm_normal.maxWidth() != fm_italic.maxWidth() || 
		fm_normal.maxWidth() != fm_boldit.maxWidth() || 
		fm_normal.maxWidth() != fm_bold.maxWidth()) {
		qDebug() << __func__ << f.family() << "Average and Maximum font width mismatch between font types";
		return true;
	}

	return false;
}

/**
 * Get an icon for a given name
 *
 * Icons are loaded in the following order:
 * 1. If the name is a Vim icon name - map it into a theme name
 * 2. If the system theme has the icon - use it
 * 3. If not - load the icon from icons.qrc
 * 4. As last resort - use a Qt standard icon
 */
QIcon VimWrapper::icon(const QString& name)
{

	QIcon icon;

	icon = QIcon::fromTheme(name.toLower(), QIcon(":/icons/" + name + ".png"));
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

QString VimWrapper::convertFrom(const QByteArray& arr)
{
	return convertFrom( arr.data(), arr.size());
}

QString VimWrapper::convertFrom(const char_u *s, int size)
{
	return convertFrom( (char*)s, size);
}

void VimWrapper::setFullscreen(bool on)
{
	if (on) {
		p_fullscreen = TRUE;
	} else {
		p_fullscreen = FALSE;
	}
}
