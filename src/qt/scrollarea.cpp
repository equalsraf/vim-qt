#include "scrollarea.moc"

#include <QWidget>
#include <QGridLayout>

ScrollArea::ScrollArea(QWidget *parent)
:QWidget(parent), m_widget(NULL)
{
	setAutoFillBackground(true);
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0,0,0,0);
	m_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
}

void ScrollArea::setWidget(QWidget *w)
{
	if ( w == m_widget || !w ) {
		return;
	}

	m_widget = w;
	m_layout->addWidget(m_widget, 0, 1);
}

void ScrollArea::setBackgroundColor(const QColor& c)
{
	QPalette p = palette();
	p.setColor(QPalette::Window, c);
	setPalette(p);
}

