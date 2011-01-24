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

protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent ( QKeyEvent *);
	virtual void closeEvent(QCloseEvent *event);
	QPoint mapText(int row, int col);


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

#endif
