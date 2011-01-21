#ifndef __QVIMSHELL__
#define __QVIMSHELL__

#include <QtGui>

class QVimShell: public QGraphicsView
{
	Q_OBJECT
public:
	QVimShell(QWidget *parent=0);

	void drawString(int row, int col, const QString& str, int flags);

public slots:
	void setBackground(const QColor&);
	void setForeground(const QColor&);
	void clearAll();
	void clearBlock(int row1, int col1, int row2, int col2);

protected:
	void resizeEvent(QResizeEvent *);
	void keyPressEvent ( QKeyEvent *);

private:
	QBrush m_foreground;

};

#endif
