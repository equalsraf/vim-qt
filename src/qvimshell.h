#ifndef __QVIMSHELL__
#define __QVIMSHELL__

#include <QtGui>

extern "C" {
#include "vim.h"
}

class QVimShell: public QWidget
{
	Q_OBJECT

public:
	QVimShell(gui_T* gui, QWidget *parent=0);

	void drawString(int row, int col, const QString& str, int flags);
	void drawHollowCursor(const QColor&);
	void drawPartCursor(const QColor&, int, int);


	void deleteLines(int row, int num_lines);
	void insertLines(int row, int num_lines);
	bool hasInput();

public slots:
	void setBackground(const QColor&);
	void setForeground(const QColor&);
	void setSpecial(const QColor&);
	void setFont(const QFont&);

	void clearAll();
	void clearBlock(int row1, int col1, int row2, int col2);
	void updateSettings();
	void invertRectangle(int row, int col, int nr, int nc);
	virtual void closeEvent(QCloseEvent *event);


protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent ( QKeyEvent *);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event );

	QPoint mapText(int row, int col);
	QRect mapBlock(int row1, int col1, int row2, int col2);

	unsigned int vimModifiers(Qt::KeyboardModifiers);
	bool specialKey(int, char[3]);
	virtual void paintEvent( QPaintEvent *);
	QByteArray convert(const QString& s);

private:
	QColor m_foreground;
	QColor m_background;
	QBrush m_special;
	gui_T *m_gui;
	QFont m_font;

	long m_blinkWaitTime, m_blinkOnTime, m_blinkOffTime;

	enum blink_state{BLINK_NONE, BLINK_ON, BLINK_OFF};
	blink_state blinkState;

	QPixmap pixmap;
	QPainter *pm_painter;
	volatile bool m_input;
};

struct special_key
{
	int key_sym;
	char code0;
	char code1;
};

static const struct special_key special_keys[] =
{
	{Qt::Key_Up,	'k', 'u'},
	{Qt::Key_Down,	'k', 'd'},
	{Qt::Key_Left,	'k', 'l'},
    	{Qt::Key_Right,	'k', 'r'},

    {Qt::Key_F1,	'k', '1'},
    {Qt::Key_F2,	'k', '2'},
    {Qt::Key_F3,	'k', '3'},
    {Qt::Key_F4,	'k', '4'},
    {Qt::Key_F5,	'k', '5'},
    {Qt::Key_F6,		'k', '6'},
    {Qt::Key_F7,		'k', '7'},
    {Qt::Key_F8,		'k', '8'},
    {Qt::Key_F9,		'k', '9'},
    {Qt::Key_F10,		'k', ';'},
    {Qt::Key_F11,		'F', '1'},
    {Qt::Key_F12,		'F', '2'},
    {Qt::Key_F13,		'F', '3'},
    {Qt::Key_F14,		'F', '4'},


//    {Qt::Key_Delete,	'k', 'b'},
    {Qt::Key_Insert,	'k', 'i'},
    {Qt::Key_Home,	'k', 'h'},
    {Qt::Key_End,	'@', '7'},
/*  {xk_prior,		'k', 'p'}, */
/*  {xk_next,		'k', 'n'}, */
/*  {xk_print,		'%', '9'}, */

    {Qt::Key_PageUp,	'k', 'p'},
    {Qt::Key_PageDown,	'k', 'n'},


    /* End of list marker: */
    {0, 0, 0}
};

#endif
