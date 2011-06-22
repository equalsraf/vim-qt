#include "vimscrollbar.moc"

VimScrollBar::VimScrollBar(scrollbar_T *sbar, Qt::Orientation o, QWidget *parent)
:QScrollBar(o, parent), sb(sbar), index(-1)
{
	setAutoFillBackground(true);

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
	index = idx;
}
