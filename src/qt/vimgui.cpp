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
	if (gui.back_pixel) {
		return *(gui.back_pixel);
	}

	return QColor();
}

QColor VimGui::normalColor()
{
	if (gui.norm_pixel) {
		return *(gui.norm_pixel);
	}

	return QColor();
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

