#include "vimqtmenu.moc"

VimQtMenu::VimQtMenu(vimmenu_T *menu, QWidget *w)
:name(VimWrapper::convertFrom(menu->name)),
	dname(VimWrapper::convertFrom(menu->dname)), 
	QMenu(w), enableMnemonic(true)
{
	setTitle(name);
}

void VimQtMenu::setEnableMnemonic(bool enabled)
{
	if (enabled == enableMnemonic) {
		return;
	}

	enableMnemonic = enabled;
	if ( enabled ) {
		setTitle(name);
	} else {
		setTitle(dname);
	}
}

