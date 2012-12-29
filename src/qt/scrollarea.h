#ifndef VIM_GUI_QT_SCROLLAREA
#define VIM_GUI_QT_SCROLLAREA

#include <QWidget>
#include <QGridLayout>

class ScrollArea: public QWidget
{
	Q_OBJECT
public:
	ScrollArea(QWidget *parent=0);
	void setWidget(QWidget *widget);

public slots:
	void setBackgroundColor(const QColor&);

private:
	QWidget *m_widget;
	QGridLayout *m_layout;

};

#endif
