#include "vimgui.h"


QPoint VimGui::mapText(int row, int col) 
{ 
	return QPoint( gui.char_width*col, gui.char_height*row );
}

QPoint VimGui::cursorPosition() 
{ 
	return mapText(gui.cursor_row, gui.cursor_col);
}

QRect VimGui::mapBlock(int row1, int col1, int row2, int col2)
{
	QPoint tl = mapText( row1, col1 );
	QPoint br = mapText( row2+1, col2+1);
	br.setX( br.x()-1 );
	br.setY( br.y()-1 );

	return QRect(tl, br);
}

QColor VimGui::backgroundColor()
{
	return fromColor(gui.back_pixel);
}

QColor VimGui::normalColor()
{
	return fromColor(gui.norm_pixel);
}

int VimGui::charWidth()
{
	return gui.char_width;
}

int VimGui::charHeight()
{
	return gui.char_height;
}

QFont VimGui::normalFont()
{
	if ( gui.norm_font ) {
		return *(gui.norm_font);
	}

	return QFont();
}

int VimGui::charCellWidth(const QChar& c)
{
	int len = utf_char2cells(c.unicode());
	if ( len <= 2 ) {
		return len;
	}

	return 0;
}

int VimGui::stringCellWidth(const QString& s)
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

QIcon VimGui::iconFromTheme(const QString& name)
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
QIcon VimGui::icon(const QString& name)
{
	QIcon icon = iconFromTheme(name);

	if ( icon.isNull() ) {
		return QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon);
	}

	return icon;
}


QColor
VimGui::fromColor(long color)
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
VimGui::toColor(const QColor& c)
{
	return ((long)c.red() << 16) + ((long)c.green() << 8) + ((long)c.blue());
}


