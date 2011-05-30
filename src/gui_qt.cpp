#include <Qt/QtGui>
#include "qvimshell.h"
#include "mainwindow.h"
#include "vimaction.h"
#include "vimscrollbar.h"

extern "C" {

#include "vim.h"

static QVimShell *vimshell = NULL;
static MainWindow *window = NULL;

static QColor foregroundColor;
static QColor backgroundColor;
static QColor specialColor;

//
// A mutex guard against concurrent execution 
// of QApplication::processEvents()
//
static QMutex loop_guard;

/**
 * Map a row/col coordinate to a point in widget coordinates
 */
static QPoint 
mapText(int row, int col)
{
	return QPoint( gui.char_width*col, gui.char_height*row );
}

/**
 * Map an area in row/col(inclusive) coordinates into
 * widget coordinates
 */
static QRect 
mapBlock(int row1, int col1, int row2, int col2)
{
	QPoint tl = mapText( row1, col1 );
	QPoint br = mapText( row2+1, col2+1);
	br.setX( br.x()-1 );
	br.setY( br.y()-1 );

	return QRect(tl, br);
}

/**
 * Raise application window
 */
void
gui_mch_set_foreground()
{
	window->activateWindow();
	window->raise();
}

/**
 * Get the font with the given name
 *
 * FIXME: implement giveErrorIfMissing
 */
GuiFont
gui_mch_get_font(char_u *name, int giveErrorIfMissing)
{
	QString family = (char*)name;
	QFont *font = new QFont();
	font->setStyleHint(QFont::Monospace);

	bool ok;
	int size = family.section(' ', -1).trimmed().toInt(&ok);

	if ( ok ) {
		font->setPointSize(size);
		QString realname = family.section(' ', 0, -2).trimmed();
		font->setFamily(realname);
	} else {
		return NULL;
	}

	font->setKerning(false);
	font->setFixedPitch(true);
	font->setBold(false);
	font->setItalic(false);

	return font;
}

/**
 * Get the name of the given font
 */
char_u *
gui_mch_get_fontname(GuiFont font, char_u  *name)
{
	if (font == NULL) {
		return NULL;
	}

	return (char_u *)font->family().constData();
}

/**
 * Free font object
 */
void
gui_mch_free_font(GuiFont font)
{
	free(font);
}

/**
 * Trigger the visual bell
 */
void
gui_mch_flash(int msec)
{
	qDebug() << __func__ << msec;
	QApplication::alert(window, msec);
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
	QMutexLocker l(&loop_guard);
	if ( wtime == -1 ) {
		QApplication::processEvents( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeSocketNotifiers);
		return OK;
	} else {
		//
		// FIXME
		// This is, evidently, broken. We should block until we get an event or the given
		// time expires. Since we have no practical way to block Qt for a certain time step
		// instead we wait indefinitely. In practice this works because vim likes pass in
		// large time slots(4s).
		//
		// @see gui_mch_update

		QTime t;
		t.start();
		do {
			QApplication::processEvents( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeSocketNotifiers);
			if ( vimshell->hasInput() ) {
				return OK;
			}
		} while( t.elapsed() < wtime );
	}
	return FAIL;
}

/**
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
void
gui_mch_update()
{
	//
	// We used to process Events here, however Qt
	// cannot handle recursive calls to resizeEvent.
	// The end result was:
	//
	// gui_mch_wait_for_chars -> resizeEvent -> gui_resize_shell-> (??)
	// -> gui_mch_update -> (!crash!)
	//
	// Now we enforce that only one of (wait_for_chars/mch_update) may
	// call process events at a time
	//
	if (loop_guard.tryLock()) {
		QApplication::processEvents();
		loop_guard.unlock();
	}
}


/** 
 * Flush any output to the screen 
 */
void
gui_mch_flush()
{
	// Is this necessary?
	QApplication::flush();
}

/**
 * Set the foreground color
 */
void
gui_mch_set_fg_color(guicolor_T	color)
{
	if ( color == NULL ) {
		return;
	}
	foregroundColor = *color;
}

/**
 * Set the current text background color.
 */
void
gui_mch_set_bg_color(guicolor_T color)
{
	if ( color == NULL ) {
		return;
	}

	// The shell needs a hint background color
	// to paint the back when resizing
	backgroundColor = *color;
}


/**
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 *
 * FIXME: We dont blink, we simply update the cursor
 */
void
gui_mch_start_blink()
{
	gui_update_cursor(TRUE, FALSE);
}

/**
 * Start the cursor blinking.
 *
 * FIXME: We dont blink, we simply update the cursor
 */
void
gui_mch_stop_blink()
{
	gui_update_cursor(TRUE, FALSE);
}

/**
 * Sound the bell
 */
void
gui_mch_beep() 
{
	QApplication::beep();
}

/**
 * Clear the entire shell, i.e. paint it with the
 * background color
 */
void
gui_mch_clear_all()
{
	PaintOperation op;
	op.type = CLEARALL;
	op.color = *(gui.back_pixel);
	vimshell->queuePaintOp(op);
}

/**
 * Initialise vim to use the font "font_name".  If it's NULL, pick a default
 * font.
 * If "fontset" is TRUE, load the "font_name" as a fontset.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
int
gui_mch_init_font(char_u *font_name, int do_fontset)
{
	QFont *qf = gui_mch_get_font(font_name, 0);
	if ( qf == NULL ) {
		return FAIL;
	}

	QFontMetrics metric( *qf );

	if ( metric.averageCharWidth() != metric.maxWidth() ) {
		qDebug() << "Warning, fake monospace font?";
	}

	gui.norm_font = qf;
	gui.char_width = metric.width(" ");
	gui.char_height = metric.height();
	gui.char_ascent = metric.ascent();
	vimshell->setCharWidth(gui.char_width);

	return OK;
}

/**
 * Get current mouse coordinates in text window.
 */
void
gui_mch_getmouse(int *x, int *y)
{
	QPoint pos = window->mapFromGlobal( QCursor::pos() );
	*x = pos.x();
	*y = pos.y();
}

/**
 * Move the mouse pointer to the given position
 */
void
gui_mch_setmouse(int x, int y)
{
	QPoint pos = window->mapToGlobal(QPoint(x, y));
	QCursor::setPos(pos.x(), pos.y());
}

/* Table for shape IDs.  Keep in sync with the mshape_names[] table in
 * misc2.c! */
static Qt::CursorShape mshape_ids[] =
{
    Qt::ArrowCursor,
    Qt::BlankCursor,
    Qt::IBeamCursor,
    Qt::SizeVerCursor,
    Qt::SizeFDiagCursor,
    Qt::SizeHorCursor,
    Qt::SizeFDiagCursor,
    Qt::BusyCursor,
    Qt::ForbiddenCursor,
    Qt::CrossCursor,
    Qt::OpenHandCursor,
    Qt::ClosedHandCursor,
    Qt::ArrowCursor,
    Qt::WhatsThisCursor,
    Qt::ArrowCursor,
    Qt::UpArrowCursor
};

/**
 * Set mouse pointer shape
 */
void
mch_set_mouse_shape(int shape)
{
	vimshell->setCursor(QCursor(mshape_ids[shape]));
}


/**
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

/**
 * Clear the block with the given coordinates
 */
void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
	QRect rect = mapBlock(row1, col1, row2, col2); 

	PaintOperation op;
	op.type = FILLRECT;
	op.color = *(gui.back_pixel);
	op.rect = rect;
	vimshell->queuePaintOp(op);
}

/**
 * Clear the shell margin
 *
 * The shell is a grid whose size is a multiple of its column/row size.
 * Sometimes an extra margin is in place, this paints that margin.
 */
static void
clear_shell_border()
{
	QPoint tl, br;

	tl.setX(0);
	tl.setY( vimshell->height() - (vimshell->height() % gui.char_height ));
	
	br.setX(vimshell->width());
	br.setY(vimshell->height());

	PaintOperation op;
	op.type = FILLRECT;
	op.color = *(gui.back_pixel);
	op.rect = QRect(tl, br);
	vimshell->queuePaintOp(op);
}

/**
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
void
gui_mch_insert_lines(int row, int num_lines)
{
	QRect scrollRect = mapBlock(row, gui.scroll_region_left, 
					gui.scroll_region_bot, gui.scroll_region_right);

	PaintOperation op1;
	op1.type = SCROLLRECT;
	op1.rect = scrollRect;
	op1.pos = QPoint(0, num_lines*gui.char_height);
	op1.color = *(gui.back_pixel);
	vimshell->queuePaintOp(op1);

	clear_shell_border();
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
void
gui_mch_delete_lines(int row, int num_lines)
{
	// This used to be Bottom+1 and right+1
	QRect scrollRect = mapBlock(row, gui.scroll_region_left, 
					gui.scroll_region_bot, gui.scroll_region_right);

	PaintOperation op;
	op.type = SCROLLRECT;
	op.rect = scrollRect;
	op.pos = QPoint(0, -num_lines*gui.char_height);
	op.color = *(gui.back_pixel);
	vimshell->queuePaintOp(op);
}


/**
 * Get the size of the screen where the application window is placed
 */
void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
	QDesktopWidget *dw = QApplication::desktop();

	QRect geo = dw->screenGeometry(window);
	*screen_w = geo.width();
	*screen_h = geo.height();
}

/**
 * Initialize vim
 *
 * - Set default colors
 * - Create window and shell
 * - Read settings
 */
int
gui_mch_init()
{
	/* Colors */
	gui.norm_pixel = new QColor(Qt::black);
	gui.back_pixel = new QColor(Qt::white);

	set_normal_colors();

	gui_check_colors();
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;

	// Clipboard - the order matters, for safety
	clip_plus.clipboardMode = QClipboard::Selection;
	clip_star.clipboardMode = QClipboard::Clipboard;

	highlight_gui_started();

	window = new MainWindow(&gui);

	QSettings settings("Vim", "qVim");
	settings.beginGroup("mainwindow");
	window->restoreState( settings.value("state").toByteArray() );
	window->resize( settings.value("size", QSize(400, 400)).toSize() );
	settings.endGroup();

	vimshell = window->vimShell();
	vimshell->setBackground(*(gui.back_pixel));

	return OK;
}

/**
 *
 * FIXME: We dont blink
 */
void
gui_mch_set_blinking(long waittime, long on, long off)
{
}

/**
 * Initialise Qt, pass in command line arguments
 */
void
gui_mch_prepare(int *argc, char **argv)
{
	QApplication *app = new QApplication(*argc, argv);
#ifdef Q_WS_X11
	QColor::setAllowX11ColorNames(true);
#endif
	QVimShell::setupColorMap();
}

/**
 * Resize the shell
 */
void
gui_mch_set_shellsize(int width, int height, int min_width, int min_height,
		    int base_width, int base_height, int direction)
{
	//
	// We don't actually resize the shell here, instead we
	// call Qt to do it for us.
	//
	gui_resize_shell(vimshell->size().width(), vimshell->size().height());
	gui_mch_update();
}

/**
 * Called when the foreground or background color has been changed.
 */
void
gui_mch_new_colors()
{
	if ( vimshell && gui.back_pixel ) {
		vimshell->setBackground(*(gui.back_pixel));
	}
}

/**
 * Move the application window
 */
void
gui_mch_set_winpos(int x, int y)
{
	window->move(x, y);
}

/**
 * Set the window title and icon.
 *
 * FIXME: We don't use the icon argument
 */
void
gui_mch_settitle(char_u *title, char_u *icon)
{
	if ( title != NULL ) {
		window->setWindowTitle( vimshell->convertFrom((char*)title) );
	}
}

/**
 * Show/Hide the mouse pointer
 */
void
gui_mch_mousehide(int hide)
{
	if ( hide ) {
		QApplication::setOverrideCursor(Qt::BlankCursor);
	} else {
		QApplication::restoreOverrideCursor();
	}
}

/**
 * Adjust gui.char_height (after 'linespace' was changed).
 */
int
gui_mch_adjust_charheight()
{
	QFontMetrics metric( *gui.norm_font );

	gui.char_height = metric.height();
	gui.char_ascent = metric.ascent();

	return OK;
}


/**
 * Return OK if the key with the termcap name "name" is supported.
 */
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

/**
 * Iconify application window
 */
void
gui_mch_iconify()
{
	window->showMinimized();
}


/**
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
void
gui_mch_invert_rectangle(int row, int col, int nr, int nc)
{
	QRect rect = mapBlock(row, col, row+nr-1, col +nc-1);

	PaintOperation op;
	op.type = INVERTRECT;
	op.rect = rect;
	vimshell->queuePaintOp(op);
}

/**
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


/**
 * Close the application
 */
void
gui_mch_exit(int rc)
{
	QSettings settings("Vim", "qVim");
	settings.beginGroup("mainwindow");
	settings.setValue("state", window->saveState());
	settings.setValue("size", window->size());
	settings.endGroup();

	QApplication::quit();
}

/**
 * Check if the GUI can be started..
 * Return OK or FAIL.
 *
 * FIXME: We cannot check Qt safely(without aborting) so we always say yes
 */
int
gui_mch_init_check()
{
	return OK;
}

/* Clipboard */

/**
 * Own the selection and return OK if it worked.
 * 
 */
int
clip_mch_own_selection(VimClipboard *cbd)
{
}

/**
 * Disown the selection.
 */
void
clip_mch_lose_selection(VimClipboard *cbd)
{
	qDebug() << __func__;
}

/**
 * Send the current selection to the clipboard
 */
void
clip_mch_set_selection(VimClipboard *cbd)
{

	int type;
	long size;
	char_u *str = NULL;

	type = clip_convert_selection(&str, (long_u *)&size, cbd);

	if (type >= 0) {
		QClipboard *clip = QApplication::clipboard();
		clip->setText( vimshell->convertFrom((char *)str, size), (QClipboard::Mode)cbd->clipboardMode);
	}

	vim_free(str);
}

/**
 * Get selection from clipboard
 *
 */
void
clip_mch_request_selection(VimClipboard *cbd)
{

	QClipboard *clip = QApplication::clipboard();
	QByteArray text = vimshell->convertTo(clip->text( (QClipboard::Mode)cbd->clipboardMode));

	char_u	*buffer;
	buffer = lalloc( text.size(), TRUE);
	if (buffer == NULL)
		return;

	for (int i = 0; i < text.size(); ++i) {
		buffer[i] = text[i];
	}

	clip_yank_selection(MCHAR, buffer, text.size(), cbd);
	vim_free(buffer);
}

/**
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

/**
 * Draw a cursor without focus.
 */
void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
	int w = gui.char_width;
	int h = gui.char_height;

	QPoint tl = QPoint(FILL_X(gui.col), 
			FILL_Y(gui.row) + gui.char_height-h );
	QPoint br = QPoint(FILL_X(gui.col)+w, 
			FILL_Y(gui.row)+gui.char_height);
	QRect rect(tl, br);

	PaintOperation op;
	op.type = DRAWRECT;
	op.rect = rect;
	op.color = *color;
	vimshell->queuePaintOp(op);
}

/**
 * Draw part of a cursor, only w pixels wide, and h pixels high.
 */
void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
	int x, y;


#ifdef FEAT_RIGHTLEFT
	if ( CURSOR_BAR_RIGHT ) {
		x = (gui.col+1)*gui.char_width- w;
		y = gui.row*gui.char_height;
	} else
#endif
	{
		x = gui.col*gui.char_width;
		y = gui.row*gui.char_height;
	}

	QRect rect( x, y, w, h);

	PaintOperation op;
	op.type = FILLRECT;
	op.rect = rect;
	op.color = foregroundColor;
	vimshell->queuePaintOp(op);
}

/**
 * Set the current text special color.
 */
void
gui_mch_set_sp_color(guicolor_T color) 
{
	if ( color == NULL ) {
		specialColor = QColor();
		return;
	}
	specialColor = *color;
}

/**
 * Draw a string in the shell
 */
void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*s,
    int		len,
    int		flags)
{
	QString str = vimshell->convertFrom((char *)s, len);
	
	// Font
	QFont f = vimshell->font();
	f.setBold( flags & DRAW_BOLD);
	f.setUnderline( flags & DRAW_UNDERL);
	f.setItalic( flags & DRAW_ITALIC);

	QFontMetrics fm(f);

	QPoint pos = mapText(row, col);
	QRect rect( pos.x(), pos.y(), gui.char_width*str.length(), gui.char_height);

	if (flags & DRAW_TRANSP) {
		// Do we need to do anything?
	} else {
		// Fill in the background
		PaintOperation op;
		op.type = FILLRECT;
		op.rect = rect;
		op.color = backgroundColor;
		vimshell->queuePaintOp(op);
	}

	PaintOperation op;
	op.type = DRAWSTRING;
	op.font = f;
	op.rect = rect;
	op.str = str;
	op.color = foregroundColor;
	op.undercurl = flags & DRAW_UNDERC;
	if ( op.undercurl ) { // FIXME: Refactor PaintOperation
		op.curlcolor = specialColor;
	}

	vimshell->queuePaintOp(op);
}


/**
 * Return the Pixel value (color) for the given color name.
 * Return INVALCOLOR for error.
 */
guicolor_T
gui_mch_get_color(char_u *reqname)
{
	if ( reqname == NULL ) {
		return INVALCOLOR;
	}
	QColor c = vimshell->color((char*)reqname);
	if ( c.isValid() ) {
		return new QColor(c);
	}

	return INVALCOLOR;
}

/**
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

/**
 * Show/Hide toolbar
 */
void
gui_mch_show_toolbar(int showit)
{
	if ( showit ) {
		window->showToolbar(true);
	} else {
		window->showToolbar(false);
	}
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

/**
 * Disable/Enable tearoff for all menus - I'm kind of shocked there is no trivial way to do this
 *
 * Given a widget, recursively find all submenus an set tearoff
 *
 */
static void
toggle_tearoffs(QWidget *widget, bool enable)
{
	foreach (QAction *action, widget->actions()) {
		if ( action->menu() ) {
			action->menu()->setTearOffEnabled(enable);
			toggle_tearoffs(action->menu(), enable);
		}
	}
}

/**
 * Enable/Disable tearoff for all menus
 */
void
gui_mch_toggle_tearoffs(int enable)
{
	QMenuBar *mb = window->menuBar();
	toggle_tearoffs(mb, enable != 0);
}

/**
 * Called after all menus are set,
 */
void
gui_mch_draw_menubar()
{
	gui_mch_update();
}

/**
 * Disable a menu entry
 */
void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
	if ( menu == NULL || menu->qaction == NULL )  {
		return;
	}
	menu->qaction->setEnabled( (grey == 0) );
}

void
gui_mch_new_menu_colors()
{
	qDebug() << __func__;

}

/**
 * Enable/Disable the application menubar
 */
void
gui_mch_enable_menu(int flag)
{
	if (flag) {
		window->showMenu(true);
	} else {
		window->showMenu(false);
	}
}

/**
 * Conceal menu entry
 */
void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
	if ( menu == NULL || menu->qaction == NULL ) {
		return;
	}
	menu->qaction->setVisible( hidden == 0 );
}

/**
 * Set the menubar position
 *
 * @warn We do nothing - The mainwindow handles this
 */
void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{
}

/**
 * Add a new ( menubar menu | toolbar action | )
 */
void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
	menu->qmenu = NULL;

	if ( menu_is_popup(menu->name) ) {
		return;
	} else if ( menu_is_toolbar(menu->name) ) {
		menu->qmenu = window->toolBar();
	} else if ( menu->parent == NULL ) {
		menu->qmenu = window->menuBar()->addMenu( vimshell->convertFrom((char*)menu->name) );
	} else if ( menu->parent && menu->parent->qmenu ) {
		QMenu *m = (QMenu*)menu->parent->qmenu;
		menu->qmenu = m->addMenu( vimshell->convertFrom((char*)menu->name) );
	}
}

/**
 * Add menu item to menu
 */
void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
	menu->qmenu = NULL;
	if ( menu->parent == NULL || menu->parent->qmenu == NULL ) {
		return;
	}

	if ( menu_is_toolbar(menu->parent->name) ) {
		// Toolbar
		QToolBar *b = (QToolBar*)menu->parent->qmenu;
		if (menu_is_separator(menu->name)) {
			b->addSeparator();
		} else {
			QAction *action = new VimAction( menu, window );
			b->addAction( action );
			menu->qaction = action;

			QObject::connect( action, SIGNAL(triggered()),
					vimshell, SLOT(forceInput()));
		}
	} else {
		// Menu entries
		QMenu *m = (QMenu *)menu->parent->qmenu;
		if (menu_is_separator(menu->name)) {
			m->addSeparator();
		} else {
			QAction *action = new VimAction( menu, window );
			m->addAction( action );
			menu->qaction = action;
			
			QObject::connect( action, SIGNAL(triggered()),
					vimshell, SLOT(forceInput()));

		}
	}
}

void
gui_mch_new_menu_font()
{
	qDebug() << __func__;

}

/**
 * Destroy menu 
 *
 * Remove menu from parent and delete it
 */
void
gui_mch_destroy_menu(vimmenu_T *menu)
{
	QMenu *parent;
	if ( menu->parent ) {
		parent = (QMenu*)menu->parent->qmenu;
	}

	if ( menu->qmenu != NULL ) {
		QMenu *m = (QMenu*)menu->qmenu;
		if ( parent ) {
			parent->removeAction(m->menuAction());
		}
		menu->qmenu->deleteLater();
	}

	if ( menu->qaction != NULL ) {
		QAction *a = (QAction*)menu->qaction;
		if ( parent ) {
			parent->removeAction(a);
		}
		menu->qaction->deleteLater();
	}

}

void gui_mch_show_popupmenu(vimmenu_T *menu)
{
	qDebug() << __func__;

}

/**
 * Set the menu and scrollbar colors to their default values.
 */
void
gui_mch_def_colors()
{
	gui.norm_pixel = new QColor(Qt::black);
	gui.back_pixel = new QColor(Qt::white);
	gui.def_norm_pixel = gui.norm_pixel;
	gui.def_back_pixel = gui.back_pixel;
}

//
//
// Scrollbar 
// 

void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
	sb->wid->setValue(val);
	sb->wid->setMaximum(max);
	sb->wid->setPageStep(size);
}

/**
 * Set scrollbar geometry
 */
void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int width, int height)
{
	//
	// Override scrollbar position to take margin 
	// into consideration
	//
	if ( sb->type == SBAR_RIGHT ) {
		x = vimshell->width() - width;
	} else if ( sb->type == SBAR_BOTTOM ) {
		y = vimshell->height() - height;
	}

	sb->wid->setGeometry(x, y, width, height);
}

/**
 *
 * Hide/Show scrollbar
 *
 */
void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
	sb->wid->setVisible(flag);
}

/**
 * Create a new scrollbar
 */
void
gui_mch_create_scrollbar( scrollbar_T *sb, int orient)
{
	Qt::Orientation dir;
	if ( orient == SBAR_HORIZ) {
		dir = Qt::Horizontal;
	} else {
		dir = Qt::Vertical;
	}

	VimScrollBar *widget = new VimScrollBar( sb, dir, vimshell);
	widget->setMinimumWidth(gui.scrollbar_width);

	sb->wid = widget;
}

/**
 * Destroy a scrollbar
 */
void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
	sb->wid->hide();
	sb->wid->deleteLater();
	sb->wid = NULL;
}

/**
 *
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

	return vim_strsave((char_u *) vimshell->convertTo(file).data());
}


/**
 * Open a dialog window
 */
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
		QStringList b_string;

		b_string = vimshell->convertFrom((char*)buttons).split(DLG_BUTTON_SEP);

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

//
// TabLine 
//

/**
 * Show or hide the tabline.
 */
void
gui_mch_show_tabline(int showit)
{
	window->showTabline(showit != 0);
	gui_mch_update();
}

/**
 * Return TRUE when tabline is displayed.
 */
int
gui_mch_showing_tabline(void)
{
	if ( window->tablineVisible() ) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Update the labels of the tabline.
 */
void
gui_mch_update_tabline(void)
{
	tabpage_T *tp;
	int current = 0;
	int nr = 0;

	for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, nr++)
	{
		if (tp == curtab) {
			current = nr;
		}

		get_tabline_label(tp, FALSE);
		char_u *labeltext = CONVERT_TO_UTF8(NameBuff);
		window->setTab( nr, vimshell->convertFrom((char*)labeltext));
		CONVERT_TO_UTF8_FREE(labeltext);
	}
	window->removeTabs(nr);
	window->setCurrentTab(current);
}

/**
 * Change the current tab
 */
void
gui_mch_set_curtab(int nr)
{
	window->setCurrentTab(nr-1);
}

/**
 * Launch font selection dialog
 * @oldval is the name of the current font
 *
 * @return The name of the font or NULL on cancel 
 */
char_u *
gui_mch_font_dialog(char_u *oldval)
{
	bool ok;
	QFont f = QFontDialog::getFont(&ok, QFont((char*)oldval), window);
	if ( ok ) {
		QByteArray text = vimshell->convertTo( QString("%1 %2").arg(f.family()).arg(f.pointSize()) );

		char_u	*buffer;
		buffer = lalloc( text.size(), TRUE);
		for (int i = 0; i < text.size(); ++i) {
			buffer[i] = text[i];
		}
		return buffer;
	}

	return NULL;
}

} // extern "C"
