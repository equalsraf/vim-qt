#include "vimaction.moc"

/**
 * :help builtin-tools
 */
QStringList VimAction::iconNames = QStringList()
	<< "document-new"	// 00  New		open new window
	<< "document-open"	// 01  Open		browse for file to open in current window
	<< "document-save"	// 02  Save		write buffer to file
	<< "edit-undo"		// 03  Undo		undo last change
	<< "edit-redo"		// 04  Redo		redo last undone change
	<< "edit-cut"		// 05  Cut		delete selected text to clipboard
	<< "edit-copy"		// 06  Copy		copy selected text to clipboard
	<< "edit-paste"		// 07  Paste		paste text from clipboard
	<< "document-print"	// 08  Print		print current buffer
	<< "help-contents"	// 09  Help		open a buffer on Vim's builtin help
	<< "edit-find"		// 10  Find		start a search command
	<< "document-save-all"	// 11  SaveAll		write all modified buffers to file
	<< "document-save-as"	// 12  SaveSesn		write session file for current situation
	<< "folder-new"		// 13  NewSesn		write new session file
	<< "folder-open"	// 14  LoadSesn		load session file // FIXME
	<< "system-run"		// 15  RunScript	browse for file to run as a Vim script
	<< "edit-find-replace"	// 16  Replace		prompt for substitute command
	<< "window-close"	// 17  WinClose		close current window
	<< ""			// 18  WinMax		make current window use many lines // FIXME
	<< ""			// 19  WinMin		make current window use few lines // FIXME
<< "view-split-left-right"	// 20  WinSplit		split current window
	<< "utilities-terminal"	// 21  Shell		start a shell
	<< "go-previous"	// 22  FindPrev		search again, backward
	<< "go-next"		// 23  FindNext		search again, forward
	<< "help-faq"		// 24  FindHelp		prompt for word to search help for
	<< "run-build"		// 25  Make		run make and jump to first error
	<< "go-jump"		// 26  TagJump		jump to tag under the cursor
	<< "table"		// 27  RunCtags		build tags for files in current directory
<< "view-split-top-bottom"	// 28  WinVSplit	split current window vertically
	<< "zoom-fit-height"	// 29  WinMaxWidth	make current window use many columns
	<< "zoom-fit-width"	// 30  WinMinWidth	make current window use few columns
	;

VimAction::VimAction(vimmenu_T* menu, QObject *parent)
:QAction(parent), m_menu(menu)
{
	if ( menu_is_toolbar(menu->parent->name) ) {
		// FIXME: add support for iconfile
		if ( menu->iconidx >= 0 && menu->iconidx <iconNames.size() ) {
			setIcon( VimWrapper::icon(iconNames.at(menu->iconidx)) );
		}

		setToolTip(VimWrapper::convertFrom(menu->strings[MENU_INDEX_TIP]));
	} else {
		setText( VimWrapper::convertFrom(menu->name) );
	}
	connect(this, SIGNAL(triggered()),
			this, SLOT(actionTriggered()));
}

void VimAction::actionTriggered()
{
	gui_menu_cb(m_menu);
}
