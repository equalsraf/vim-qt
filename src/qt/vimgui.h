#ifndef __VIM_QT_GUI__
#define __VIM_QT_GUI__

#include <Qt/QtGui>

extern "C" {
#include "vim.h"
}

class VimGui
{
public:
	/**
	 * Map a row/col coordinate to a point in widget coordinates
	 */
	static QPoint mapText(int row, int col);

	/**
	 * Returns the current cursor coordinates(top left corner)
	 */
	static QPoint cursorPosition();

	/**
	 * Map an area in row/col(inclusive) coordinates into
	 * widget coordinates
	 */
	static QRect mapBlock(int row1, int col1, int row2, int col2);

	/**
	 * The current background pixel color
	 */
	static QColor backgroundColor();

	/**
	 * Normal text color
	 */
	static QColor normalColor();

	static int charWidth();
	static int charHeight();

	/**
	 * Gui normal font
	 */
	static QFont normalFont();

	static int stringCellWidth(const QString&);
	static int charCellWidth(const QChar&);
};

#endif
