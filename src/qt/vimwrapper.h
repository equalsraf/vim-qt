#ifndef __VIM_QT_VIM__
#define __VIM_QT_VIM__

#include <QObject>
#include <QIcon>
#include <QUrl>
#include <QWidget>
#include <QDebug>

extern "C" {
#include "vim.h"
}

#include "vimevents.h"

/**
 * VimWrapper is wrapper around Vim, it handles conversion between Qt types
 * and vim function's argument types
 *
 */
class VimWrapper
{
public:
	VimWrapper();
	
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
	static bool isFakeMonospace(QFont );

	/**
	 * Load icons
	 */
	static QIcon icon(const QString&);

	static QColor fromColor(long);
	static long toColor(const QColor&);

	/**
	 * New tab
	 */
	static void newTab(int idx=1);

	static QByteArray convertTo(const QString& s);
	static QString convertFrom(const char_u *, int size=-1);
	static QString convertFrom(const QByteArray&);

	static char_u* copy(const QByteArray&);

	/**
	 * Vim methods
	 */
	void postGuiResizeShell(int w, int h);
	void guiResizeShell(int w, int h);
	void guiShellClosed();
	void guiSendMouseEvent(int , int , int , int , unsigned int );
	void guiMouseMoved(int, int);
	void guiFocusChanged(int);

	void guiHandleDropText(const QString&);
	void guiHandleDrop(const QPoint& , unsigned int, const QList<QUrl>);

	void sendTablineEvent(int);
	void sendTablineMenuEvent(int, int);

	void updateCursor(bool force, bool clearsel);
	void undrawCursor();

	static void setFullscreen(bool on);
	void setProcessInputOnly(bool input_only);
	bool processEvents(long wtime=0, bool inputOnly=false);

protected:
	static QString convertFrom(const char *, int size=-1);
	bool hasPendingEvents();

private:
	bool m_processInputOnly;
	QList<VimEvent *> pendingEvents;
};

#endif
