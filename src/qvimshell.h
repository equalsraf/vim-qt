#ifndef __QVIMSHELL__
#define __QVIMSHELL__

#include <QtGui>

extern "C" {
#include "vim.h"
}

class QVimShell: public QGraphicsView
{
	Q_OBJECT
	Q_PROPERTY(long blinkWaitTime READ blinkWaitTime WRITE setBlinkWaitTime)
	Q_PROPERTY(long blinkOnTime READ blinkOnTime WRITE setBlinkOnTime)
	Q_PROPERTY(long blinkOffTime READ blinkOffTime WRITE setBlinkOffTime)

public:
	QVimShell(gui_T* gui, QWidget *parent=0);

	void drawString(int row, int col, const QString& str, int flags);
	void drawHollowCursor(const QColor&);
	void drawPartCursor(const QColor&, int, int);

	long blinkWaitTime();
	long blinkOnTime();
	long blinkOffTime();
	void setBlinkWaitTime(long);
	void setBlinkOnTime(long);
	void setBlinkOffTime(long);

	void deleteLines(int row, int num_lines);
	void insertLines(int row, int num_lines);


public slots:
	void setBackground(const QColor&);
	void setForeground(const QColor&);
	void setSpecial(const QColor&);
	void setFont(const QFont&);

	void clearAll();
	void clearBlock(int row1, int col1, int row2, int col2);
	void updateSettings();
	void startBlinking();
	void stopBlinking();
	void invertRectangle(int row, int col, int nr, int nc);


protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent ( QKeyEvent *);
	virtual void closeEvent(QCloseEvent *event);
	QPoint mapText(int row, int col);
	QRect mapBlock(int row1, int col1, int row2, int col2);

	unsigned int vimModifiers(Qt::KeyboardModifiers);
	bool specialKey(int, char[3]);


protected slots:
	void blinkEvent();

private:
	QBrush m_foreground;
	QBrush m_background;
	QBrush m_special;
	gui_T *m_gui;
	QFont m_font;

	long m_blinkWaitTime, m_blinkOnTime, m_blinkOffTime;

	QTimer blinkTimer;
	enum blink_state{BLINK_NONE, BLINK_ON, BLINK_OFF};
	blink_state blinkState;

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
    {Qt::Key_Right,		'k', 'r'},
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
    {Qt::Key_F15,		'F', '5'},
    {Qt::Key_F16,		'F', '6'},
    {Qt::Key_F17,		'F', '7'},
    {Qt::Key_F18,		'F', '8'},
    {Qt::Key_F19,		'F', '9'},
    {Qt::Key_F20,		'F', 'A'},
    {Qt::Key_F21,		'F', 'B'},
    {Qt::Key_Pause,		'F', 'B'}, /* Pause == F21 according to netbeans.txt */
    {Qt::Key_F22,		'F', 'C'},
    {Qt::Key_F23,		'F', 'D'},
    {Qt::Key_F24,		'F', 'E'},
    {Qt::Key_F25,		'F', 'F'},
    {Qt::Key_F26,		'F', 'G'},
    {Qt::Key_F27,		'F', 'H'},
    {Qt::Key_F28,		'F', 'I'},
    {Qt::Key_F29,		'F', 'J'},
    {Qt::Key_F30,		'F', 'K'},
    {Qt::Key_F31,		'F', 'L'},
    {Qt::Key_F32,		'F', 'M'},
    {Qt::Key_F33,		'F', 'N'},
    {Qt::Key_F34,		'F', 'O'},
    {Qt::Key_F35,		'F', 'P'},
    {Qt::Key_Help,		'%', '1'},

    {Qt::Key_Backspace,	'k', 'b'},
    {Qt::Key_Insert,	'k', 'I'},
    {Qt::Key_Delete,	'k', 'D'},
    {Qt::Key_Backtab,	'k', 'B'},
    {Qt::Key_Clear,		'k', 'C'},
    {Qt::Key_Home,		'k', 'h'},
    {Qt::Key_End,		'@', '7'},
    /* Keypad keys: */

    /* End of list marker: */
    {0, 0, 0}
};

#endif
