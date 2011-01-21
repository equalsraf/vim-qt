#include <Qt/QtGui>
#include "qvimshell.h"

extern "C" {

#include "vim.h"

//QWidget *window = NULL;
//static QGraphicsView *window = NULL;
static QVimShell *window = NULL;
static QBrush fg_brush;


void
gui_mch_set_foreground()
{
	window->activateWindow();
	window->raise();
}

GuiFont
gui_mch_get_font(char_u *name, int giveErrorIfMissing)
{
	QFont *font = new QFont((char*)name);
	return font;
}

char_u *
gui_mch_get_fontname(GuiFont font, char_u  *name)
{
	if (font == NULL) {
		return NULL;
	}

	return NULL; // FIXME: return family name
}

void
gui_mch_free_font(GuiFont font)
{
	free(font);
}

void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
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
	if ( wtime > 0 ) {
		QApplication::processEvents( QEventLoop::WaitForMoreEvents, wtime);
		return FAIL;
	}
	if ( wtime == -1 ) {
		QApplication::processEvents( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeSocketNotifiers);
		return OK;
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

void
gui_mch_flush()
{
	QApplication::flush();
}

void
gui_mch_set_fg_color(guicolor_T	color)
{
	if ( color == NULL ) {
		window->setForeground(QColor());
		return;
	}

	window->setForeground(*color);
}

/*
 * Set the current text background color.
 */
void
gui_mch_set_bg_color(guicolor_T color)
{
	if ( color == NULL ) {
		window->setBackground(QColor());
		return;
	}

	window->setBackground(*color);
}


/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
void
gui_mch_start_blink()
{
//	window->startBlinking();
}

void
gui_mch_stop_blink()
{
//	window->stopBlinking();
}


void
gui_mch_beep() 
{
	QApplication::beep();
}

void
gui_mch_clear_all()
{
	window->clearAll();
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
	gui.char_width  = metric.width("_");
	gui.char_height = metric.height();
	gui.char_ascent = metric.ascent();

	return OK;
}

/*
 * Get current mouse coordinates in text window.
 */
void
gui_mch_getmouse(int *x, int *y)
{

	QPoint pos = window->mapFromGlobal( QCursor::pos() );
	// FIXME: check for error
	*x = pos.x();
	*y = pos.y();
}

void
gui_mch_setmouse(int x, int y)
{
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
	window->clearBlock(row1, col1, row2, col2);
}

void
gui_mch_insert_lines(int row, int num_lines)
{
	qDebug() << __func__;

}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
void
gui_mch_delete_lines(int row, int num_lines)
{
	qDebug() << __func__;
	//window->clearBlock(row, gui.scroll_region_right, row+num_lines-1, gui.scroll_region_right);
}



void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
	QDesktopWidget *dw = QApplication::desktop();

	QRect geo = dw->screenGeometry(window);
	*screen_w = geo.width();
	*screen_h = geo.height();
}

int
gui_mch_init()
{
	fprintf(stderr, "%s\n", __func__);


	/* Colors */
	gui.norm_pixel = new QColor(Qt::black);
	gui.back_pixel = new QColor(Qt::white);

	set_normal_colors();

	gui_check_colors();
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;

	highlight_gui_started();

	QGraphicsScene *scene = new QGraphicsScene();
	window = new QVimShell(&gui);
	return OK;
}

void
gui_mch_set_blinking(long waittime, long on, long off)
{
	//qDebug() << __func__ << waittime << on << off;
	window->setBlinkWaitTime(waittime);
	window->setBlinkOnTime(on);
	window->setBlinkOffTime(off);
}

void
gui_mch_prepare(int *argc, char **argv)
{
	fprintf(stderr, "%s\n", __func__);
	QApplication *app = new QApplication(*argc, argv);
}

void
gui_mch_set_shellsize(int width, int height, int min_width, int min_height,
		    int base_width, int base_height, int direction)
{
	window->resize(width, height);
}


void
gui_mch_new_colors()
{
	if ( window != NULL ) {
		//trigger scene update with new colors
		window->updateSettings();
		window->update();

	}
}


void
gui_mch_set_winpos(int x, int y)
{
	window->move(x, y);
}

void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
}

void
gui_mch_draw_menubar()
{
}

void
gui_mch_mousehide(int hide)
{
}

int
gui_mch_adjust_charheight()
{
	QFontMetrics metric( *gui.norm_font );

	gui.char_height = metric.height();
	gui.char_ascent = metric.ascent();

	return OK;
}

int
gui_mch_haskey(char_u *name)
{
	return FAIL;
}

void
gui_mch_iconify()
{
}


void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
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


}


void
gui_mch_exit(int rc)
{
	QApplication::quit();
}

int
gui_mch_init_check()
{
	return OK;
}

int
clip_mch_own_selection(VimClipboard *cbd)
{

}

void
clip_mch_lose_selection(VimClipboard *cbd)
{
}


/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
int
gui_mch_open()
{
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
	gui_mch_set_fg_color(color);
	window->drawHollowCursor(*color);
}

void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
	gui_mch_set_fg_color(color);
	window->drawPartCursor(*color, w, h);
}


/*
 * Set the current text special color.
 */
void
gui_mch_set_sp_color(guicolor_T color) 
{
	if ( color == NULL ) {
		window->setSpecial(QColor());
		return;
	}

	window->setSpecial(*color);
}

void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*s,
    int		len,
    int		flags)
{
	//qDebug() << __func__ << len << gui.scroll_region_right;
	QString str = QString::fromUtf8((char *)s, len);
	window->drawString(row, col, str, flags);
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
	}

	return INVALCOLOR;
}


void
clip_mch_set_selection(VimClipboard *cbd)
{
}


void
clip_mch_request_selection(VimClipboard *cbd)
{
}


/*
 * Get the position of the top left corner of the window.
 */
int
gui_mch_get_winpos(int *x, int *y)
{
	QPoint pos = window->pos();

	*x = pos.x();
	*y = pos.y();

	return OK;
}


void
gui_mch_set_toolbar_pos(int x, int y, int w, int h)
{

}


void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
}



void
gui_mch_new_tooltip_font()
{
}

void
gui_mch_new_tooltip_colors()
{
}

void
gui_mch_add_menu_item(vimmenu_T menu, int idx)
{

}


void
gui_mch_show_toolbar(int showit)
{

}

int
gui_mch_compute_toolbar_height()
{
	return -1;
}

/* Menu */

void
gui_mch_toggle_tearoffs(int enable)
{
}

void
gui_mch_new_menu_colors()
{
}

void
gui_mch_enable_menu(int flag)
{
}

void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{

}

void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
}

void
gui_mch_new_menu_font()
{
}

void
gui_mch_destroy_menu(vimmenu_T *menu)
{
}

void gui_mch_show_popupmenu(vimmenu_T *menu)
{
}

void
gui_mch_def_colors()
{
	if ( gui.in_use ) {
	
	
	}
}

/* Scrollbar */

void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
	
}

void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)
{
}

void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
	if ( flag ) {
		
	} else {
		
	}
}

void
gui_mch_create_scrollbar( scrollbar_T *sb, int orient)
{
}

void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
}

void
gui_mch_set_scrollbar_colors(scrollbar_T *sb)
{

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

	QString file = QFileDialog::getOpenFileName(window, (char*)title, dir, (char*)filter);

	return NULL; // FIXME: return outcome
}

int
gui_mch_dialog(int type, char_u *title, char_u *message, char_u *buttons, int dfltbutton, char_u *textfield)
{
	return -1;
}

} // extern "C"
