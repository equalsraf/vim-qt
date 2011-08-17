#ifndef __VIM_QT_VIM__
#define __VIM_QT_VIM__

#include <Qt/QtGui>

extern "C" {
#include "vim.h"
}

class VimWrapper: public QObject
{
	Q_OBJECT
public:
	VimWrapper(QObject *parent=0);
	
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

	/**
	 * Load icons
	 */
	static QIcon iconFromTheme(const QString&);
	static QIcon icon(const QString&);

	static QColor fromColor(long);
	static long toColor(const QColor&);

	/**
	 * New tab
	 */
	static void newTab(int idx=1);

	static QByteArray convertTo(const QString& s);
	static QString convertFrom(const char *, int size=-1);


	/**
	 * Vim methods
	 */
	void guiResizeShell(int, int);
	void guiShellClosed();
	void guiSendMouseEvent(int, int);
	void guiSendMouseEvent(int , int , int , int , unsigned int );
	void guiMouseMoved(int, int);
	void guiFocusChanged(int);
	void guiHandleDrop(int, int, unsigned int, const QList<QUrl>);

	void sendTablineEvent(int);
	void sendTablineMenuEvent(int, int);

private slots:

	/**
	 * Vim methods slots
	 */
	void slot_guiResizeShell(int, int);
	void slot_guiShellClosed();
	void slot_guiSendMouseEvent(int , int , int , int , unsigned int );
	void slot_guiMouseMoved(int, int);
	void slot_guiFocusChanged(int);
	void slot_guiHandleDrop(int, int, unsigned int, const QList<QUrl>);

//	void guiMenuCb(long);

};

#endif
