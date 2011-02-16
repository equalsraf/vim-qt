#include "vimaction.moc"

VimAction::VimAction(vimmenu_T* menu, QObject *parent)
:QAction(parent), m_menu(menu)
{
	if ( menu_is_toolbar(menu->parent->name) ) {
		setIcon( QVimShell::icon(QString::fromUtf8((char*)menu->name)) );
		setToolTip(QString::fromUtf8((char*) menu->strings[MENU_INDEX_TIP]));
	} else {
		setText( QString::fromUtf8((char*)menu->name) );
	}
	connect(this, SIGNAL(triggered()),
			this, SLOT(actionTriggered()));
}

void VimAction::actionTriggered()
{
	gui_menu_cb(m_menu);
}
