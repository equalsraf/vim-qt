#include "vimscrollbar.moc"

VimScrollBar::VimScrollBar(scrollbar_T *sbar, Qt::Orientation o, QWidget *parent)
:QScrollBar(o, parent), sb(sbar), m_index(-1), m_length(0)
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
	if ( idx != m_index ) {
		m_index = idx;
		emit indexChanged(m_index);
	}
}

void VimScrollBar::setLength(int len)
{
	if ( len != m_length ) {
		m_length = len;
		emit indexChanged(m_index); // FIXME
	}
}

int VimScrollBar::length()
{
	return m_length;
}

int VimScrollBar::index() const
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

