#include <Qt/QtGui>
#include "qvimshell.h"
#include "mainwindow.h"

extern "C" {

#include "vim.h"

static QVimShell *vimshell = NULL;
static MainWindow *window = NULL;

void
gui_mch_set_foreground()
{
	window->activateWindow();
	window->raise();
}

GuiFont
gui_mch_get_font(char_u *name, int giveErrorIfMissing)
{
	qDebug() << __func__;

	QString family = (char*)name;
	QFont *font = new QFont();

	bool ok;
	int size = family.section(' ', -1).toInt(&ok);
	if ( ok ) {
		font->setPointSize(size);
		QString realname = family.section(' ', 0, -2);
		font->setFamily(realname);
	} else {
		font->setFamily(family);
	}

	return font;
}

char_u *
gui_mch_get_fontname(GuiFont font, char_u  *name)
{
	qDebug() << __func__;

	if (font == NULL) {
		return NULL;
	}

	return (char_u *)font->family().constData(); // FIXME: return family name
}

void
gui_mch_free_font(GuiFont font)
{
	free(font);
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1	    Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
int
gui_mch_wait_for_chars(long wtime)
{
	long left = wtime;	

	if ( wtime == -1 ) {
		QApplication::processEvents( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeSocketNotifiers);
	} else {
		QTime t;
		t.start();
		do {
			QApplication::processEvents( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeSocketNotifiers, wtime - t.elapsed());
			if ( vimshell->hasInput() )
				return OK;
		} while( t.elapsed() < wtime );
	}

	return FAIL;
}

/*
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
void
gui_mch_update()
{
	QApplication::processEvents();
}


/* Flush any output to the screen */
void
gui_mch_flush()
{
	// Is this necessary
	//QApplication::flush();
}

void
gui_mch_set_fg_color(guicolor_T	color)
{
	if ( color == NULL ) {
		vimshell->setForeground(QColor());
		return;
	}

	vimshell->setForeground(*color);
}

/*
 * Set the current text background color.
 */
void
gui_mch_set_bg_color(guicolor_T color)
{
	if ( color == NULL ) {
		vimshell->setBackground(QColor());
		return;
	}

	vimshell->setBackground(*color);
}


/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
void
gui_mch_start_blink()
{
	qDebug() << __func__;

	gui_update_cursor(TRUE, FALSE);

}

void
gui_mch_stop_blink()
{
	qDebug() << __func__;

	gui_update_cursor(TRUE, FALSE);
}


void
gui_mch_beep() 
{
	QApplication::beep();
}

void
gui_mch_clear_all()
{
	qDebug() << __func__;

	vimshell->clearAll();
}

void
gui_mch_flash(int msec)
{
	qDebug() << __func__;
}

/*
 * Initialise vim to use the font "font_name".  If it's NULL, pick a default
 * font.
 * If "fontset" is TRUE, load the "font_name" as a fontset.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
int
gui_mch_init_font(char_u *font_name, int do_fontset)
{
	QFont *qf = gui_mch_get_font(font_name, 0);
	QFontMetrics metric( *qf );

	gui.norm_font = qf;
	gui.char_width = metric.width("_");
	gui.char_height = metric.height();
	
	gui.char_ascent = metric.ascent();

	return OK;
}

/*
 * Get current mouse coordinates in text window.
 * FIXME: This is currently broken
 */
void
gui_mch_getmouse(int *x, int *y)
{
	qDebug() << __func__;

	QPoint pos = window->mapFromGlobal( QCursor::pos() );
	// FIXME: check for error
	*x = pos.x();
	*y = pos.y();
}

void
gui_mch_setmouse(int x, int y)
{
	qDebug() << __func__;

	QPoint pos = window->mapToGlobal(QPoint(x, y));
	QCursor::setPos(pos.x(), pos.y());
}

/*
 * Return the RGB value of a pixel as a long.
 */
long_u
gui_mch_get_rgb(guicolor_T pixel)
{
	if ( pixel != NULL ) {
		return ((pixel->red() & 0xff00) << 8) + (pixel->green() & 0xff00)
						   + ((unsigned)pixel->blue() >> 8);
	}
	return 0;
}

void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
	qDebug() << __func__;

	vimshell->clearBlock(row1, col1, row2, col2);
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
void
gui_mch_insert_lines(int row, int num_lines)
{
	qDebug() << __func__;

	vimshell->insertLines(row, num_lines);
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
void
gui_mch_delete_lines(int row, int num_lines)
{
	qDebug() << __func__;

	vimshell->deleteLines(row, num_lines);
}



void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
	qDebug() << __func__;

	QDesktopWidget *dw = QApplication::desktop();

	QRect geo = dw->screenGeometry(window);
	*screen_w = geo.width();
	*screen_h = geo.height();
}

int
gui_mch_init()
{
	qDebug() << __func__;
	/* Colors */
	gui.norm_pixel = new QColor(Qt::black);
	gui.back_pixel = new QColor(Qt::white);

	set_normal_colors();

	gui_check_colors();
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;

	highlight_gui_started();

	window = new MainWindow(&gui);
	vimshell = window->vimShell();

	return OK;
}

void
gui_mch_set_blinking(long waittime, long on, long off)
{
}

void
gui_mch_prepare(int *argc, char **argv)
{
	QApplication *app = new QApplication(*argc, argv);
}

void
gui_mch_set_shellsize(int width, int height, int min_width, int min_height,
		    int base_width, int base_height, int direction)
{
	qDebug() << __func__;

	window->resize(width, height);
}

void
gui_mch_new_colors()
{
	qDebug() << __func__;

	if ( window != NULL ) {
		vimshell->updateSettings();
	}
}

void
gui_mch_set_winpos(int x, int y)
{
	qDebug() << __func__;
	window->move(x, y);
}


void
gui_mch_settitle(char_u *title, char_u *icon UNUSED)
{
	qDebug() << __func__;
	if ( title != NULL ) {
		window->setWindowTitle( QString::fromUtf8((char*)title) );
	}
}

void
gui_mch_mousehide(int hide)
{
	qDebug() << __func__;

	if ( hide ) {
		QApplication::setOverrideCursor(Qt::BlankCursor);
	} else {
		QApplication::restoreOverrideCursor();
	}
}

int
gui_mch_adjust_charheight()
{
	qDebug() << __func__;

	QFontMetrics metric( *gui.norm_font );

	gui.char_height = metric.height();
	gui.char_ascent = metric.ascent();

	return OK;
}

int
gui_mch_haskey(char_u *name)
{
	int i;
	qDebug() << __func__;

	for (i = 0; special_keys[i].code1 != NUL; i++) {
		if (name[0] == special_keys[i].code0 &&
					 name[1] == special_keys[i].code1) {
			return OK;
		}
	}
	return FAIL;
}

void
gui_mch_iconify()
{
	window->showMinimized();
}


/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
	qDebug() << __func__;
	vimshell->invertRectangle(r, c, nr, nc);
}

/*
 * Set the current text font.
 */
void
gui_mch_set_font(GuiFont font)
{
	if (font == NULL) {
		return;
	}
	vimshell->setFont(*font);
}


void
gui_mch_exit(int rc)
{
	qDebug() << __func__;
	QApplication::quit();
}

int
gui_mch_init_check()
{
	qDebug() << __func__;
	return OK;
}

int
clip_mch_own_selection(VimClipboard *cbd)
{
	qDebug() << __func__;
	return OK;
}

void
clip_mch_lose_selection(VimClipboard *cbd)
{
	qDebug() << __func__;
}

/*
 * Send the current selection to the clipboard
 */
void
clip_mch_set_selection(VimClipboard *cbd)
{
	qDebug() << __func__;

	QClipboard *clip = QApplication::clipboard();
	clip->setText( "Hello World", QClipboard::Selection);

}

void
clip_mch_request_selection(VimClipboard *cbd)
{
	qDebug() << __func__;
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
int
gui_mch_open()
{
	qDebug() << __func__;
	if ( window != NULL ) {
		window->show();
		return OK;
	}

	return FAIL;
}

/*
 * Draw a cursor without focus.
 */
void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
	qDebug() << __func__;

	gui_mch_set_fg_color(color);
	vimshell->drawHollowCursor(*color);
}

void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
	qDebug() << __func__;

	vimshell->drawPartCursor(*color, w, h);
}


/*
 * Set the current text special color.
 */
void
gui_mch_set_sp_color(guicolor_T color) 
{
	if ( color == NULL ) {
		vimshell->setSpecial(QColor());
		return;
	}

	vimshell->setSpecial(*color);
}

void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*s,
    int		len,
    int		flags)
{
	QString str = QString::fromUtf8((char *)s, len);
	vimshell->drawString(row, col, str, flags);
}


/*
 * Return the Pixel value (color) for the given color name.
 * Return INVALCOLOR for error.
 */
guicolor_T
gui_mch_get_color(char_u *reqname)
{
	if ( QColor::isValidColor((char *)reqname) ) {
		QColor *color = new QColor((char *)reqname);
		return color;
	} else if ( qstrcmp( (char *)reqname, "Grey40" ) == 0 ) {
		QColor *color = new QColor("#666666");
		return color;
	}

	return INVALCOLOR;
}

/*
 * Get the position of the top left corner of the window.
 */
int
gui_mch_get_winpos(int *x, int *y)
{
	qDebug() << __func__;

	QPoint pos = vimshell->pos();

	*x = pos.x();
	*y = pos.y();

	return OK;
}


void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
	/* Do we need to do anything here? */
}

void
gui_mch_new_tooltip_font()
{
	qDebug() << __func__;

}

void
gui_mch_new_tooltip_colors()
{
	qDebug() << __func__;

}

void
gui_mch_show_toolbar(int showit)
{
	qDebug() << __func__;
}

int
gui_mch_compute_toolbar_height()
{
	/* Do nothing - the main window handles this */
	return 0;
}

void
gui_mch_set_toolbar_pos(int x, int y, int w, int h)
{
	/* Do nothing - the main window handles this */
}

//
// MENU
//
///////////////////

void
gui_mch_toggle_tearoffs(int enable)
{
	qDebug() << __func__;
	
}

/*
 * Called after all menus are set,
 */
void
gui_mch_draw_menubar()
{
	gui_mch_update();
}

/*
 * Disable a menu entry
 */
void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
	if ( menu == NULL || menu->qaction == NULL )  {
		return;
	}
	menu->qaction->setEnabled( !(grey != 0) );
}

void
gui_mch_new_menu_colors()
{
	qDebug() << __func__;

}

/*
 * Enable/Disable the application menubar
 */
void
gui_mch_enable_menu(int flag)
{
	qDebug() << __func__ << flag;
	if (flag) {
		window->menuBar()->show();
	} else {
		window->menuBar()->hide();
	}
}

/*
 * Conceal menu entry
 */
void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
	qDebug() << __func__ << (char*)menu->name;
	if ( menu == NULL || menu->qaction == NULL ) {
		return;
	}
	menu->qaction->setVisible( hidden != 0 );
}

void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{
	/* Do nothing - The mainwindow handles this */
}

/*
 * Add a new ( menubar menu | toolbar action | )
 */
void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
	menu->qmenu = NULL;

	if ( menu_is_popup(menu->name) ) {
		return;
	} else if ( menu_is_toolbar(menu->name) ) {
		menu->qmenu = menu->qmenu = window->addToolBar( QString::fromUtf8((char*)menu->name) );
	} else if (  menu->parent == NULL ) {
		menu->qmenu = window->menuBar()->addMenu( QString::fromUtf8((char*)menu->name) );
	}
}

void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
	menu->qmenu = NULL;
	if ( menu->parent == NULL || menu->parent->qmenu == NULL ) {
		return;
	}

	if ( menu_is_toolbar(menu->parent->name) ) {

		QToolBar *b = (QToolBar*)menu->parent->qmenu;
		if (menu_is_separator(menu->name)) {
			b->addSeparator();
		} else {
			QString tip = QString::fromUtf8((char*) menu->strings[MENU_INDEX_TIP]);
			QString name = QString::fromUtf8((char*) menu->name);
			menu->qaction = b->addAction( QVimShell::icon(name), tip);
		}
	} else {

		QMenu *m = (QMenu *)menu->parent->qmenu;
		if (menu_is_separator(menu->name)) {
			m->addSeparator();
		} else {
			menu->qaction = m->addAction( QString::fromUtf8((char*)menu->name));
		}
	}
}



void
gui_mch_new_menu_font()
{
	qDebug() << __func__;

}

/*
 * Destroy menu 
 */
void
gui_mch_destroy_menu(vimmenu_T *menu)
{
	/* Nothing to do */
	qDebug() << __func__;
	if ( menu->qmenu != NULL ) {
		menu->qmenu->deleteLater();
	}
	if ( menu->qaction != NULL ) {
		menu->qaction->deleteLater();
	}

}

void gui_mch_show_popupmenu(vimmenu_T *menu)
{
	qDebug() << __func__;

}

/*
 * Set the menu and scrollbar colors to their default values.
 */
void
gui_mch_def_colors()
{
	qDebug() << __func__;

	gui.norm_pixel = new QColor(Qt::black);
	gui.back_pixel = new QColor(Qt::white);
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;
}

/* Scrollbar */

void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
	qDebug() << __func__;
}

void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)
{
	qDebug() << __func__;

}

void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
	qDebug() << __func__;
}

void
gui_mch_create_scrollbar( scrollbar_T *sb, int orient)
{
	qDebug() << __func__;
	if ( orient == SBAR_HORIZ) {
//		sb->wid = window->horizontalScrollBar();
	} else {
//		sb->wid = window->verticalScrollBar();
	}
}

void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
	// ScrollBar is owned by the viewport, do nothing
	qDebug() << __func__;
}

void
gui_mch_set_scrollbar_colors(scrollbar_T *sb)
{
	qDebug() << __func__;

}

/*
 * Pop open a file browser and return the file selected, in allocated memory,
 * or NULL if Cancel is hit.
 *  saving  - TRUE if the file will be saved to, FALSE if it will be opened.
 *  title   - Title message for the file browser dialog.
 *  dflt    - Default name of file.
 *  ext     - Default extension to be added to files without extensions.
 *  initdir - directory in which to open the browser (NULL = current dir)
 *  filter  - Filter for matched files to choose from.
 *  Has a format like this:
 *  "C Files (*.c)\0*.c\0"
 *  "All Files\0*.*\0\0"
 *  If these two strings were concatenated, then a choice of two file
 *  filters will be selectable to the user.  Then only matching files will
 *  be shown in the browser.  If NULL, the default allows all files.
 *
 *  *NOTE* - the filter string must be terminated with TWO nulls.
 */
char_u *
gui_mch_browse(int saving, char_u *title, char_u *dflt, char_u *ext, char_u *initdir, char_u *filter)
{

	QString dir;
	if ( initdir == NULL ) {
		dir = "";
	} else {
		dir = (char*)initdir;
	}

	QString file = QFileDialog::getOpenFileName(window, (char*)title, dir, ".*");

	if ( file.isEmpty() ) {
		return NULL;
	}
	qDebug() << file;

	return vim_strsave((char_u *)file.toUtf8().data()); // FIXME: return outcome
}

int
gui_mch_dialog(int type, char_u *title, char_u *message, char_u *buttons, int dfltbutton, char_u *textfield)
{
	QMessageBox msgBox;
	msgBox.setText( (char*)message );
	msgBox.setWindowTitle( (char*)title );
	
	// Set icon
	QMessageBox::Icon icon;
	switch (type)
	{
		case VIM_GENERIC:
			icon = QMessageBox::NoIcon;
			break;
		case VIM_ERROR:
			icon = QMessageBox::Critical;
			break;
		case VIM_WARNING:
			icon = QMessageBox::Warning;
			break;
		case VIM_INFO:
			icon = QMessageBox::Information;
			break;
		case VIM_QUESTION:
			icon = QMessageBox::Question;
			break;
		default:      
			icon = QMessageBox::NoIcon;
	};
	msgBox.setIcon(icon);

	// Add buttons
	QList<QPushButton *> buttonList;
	if ( buttons != NULL ) {
		QStringList b_string = QString::fromUtf8((char*)buttons).split(DLG_BUTTON_SEP);
		QListIterator<QString> it(b_string);
		int bt=1;
		while(it.hasNext()) {
			QPushButton *b = msgBox.addButton( it.next(), QMessageBox::ApplyRole);
			buttonList.append(b);

			if ( bt == dfltbutton ) {
				b->setDefault(true);
			}
			bt++;
		}
	}

	msgBox.exec();

	if ( msgBox.clickedButton() == 0 ) {
		return 0;
	}

	int i=1;
	QListIterator<QPushButton *> it(buttonList);
	while( it.hasNext() ){
		QPushButton *b = it.next();
		if ( b == msgBox.clickedButton() ) {
			return i;
		}
		i++;
	}
	return -1;
}

} // extern "C"
