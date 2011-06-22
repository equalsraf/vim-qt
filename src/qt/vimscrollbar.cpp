#include "vimscrollbar.moc"

VimScrollBar::VimScrollBar(scrollbar_T *sbar, Qt::Orientation o, QWidget *parent)
:QScrollBar(o, parent), sb(sbar), m_index(-1)
{
	connect(this, SIGNAL(valueChanged(int)),
			this, SLOT(draggingFinished()));
}

void VimScrollBar::dragging()
{
	if (sb == NULL) {
		return;
	}

	gui_drag_scrollbar(sb, this->value(), 1);
}

void VimScrollBar::draggingFinished()
{
	if (sb == NULL) {
		return;
	}

	gui_drag_scrollbar(sb, this->value(), 0);
}

void VimScrollBar::setIndex(int idx)
{
	m_index = idx;
	emit indexChanged(m_index);
}

int VimScrollBar::index()
{
	return m_index;
}

void VimScrollBar::setVisible(bool show)
{
	bool visible = isVisible();
	QScrollBar::setVisible(show);

	if ( visible != show )
		emit visibilityChanged(show);
}

