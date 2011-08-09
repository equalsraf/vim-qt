#include <Qt/QtGui>
#include "qvimshell.h"
#include "mainwindow.h"
#include "vimaction.h"
#include "vimscrollbar.h"
#include "fontdialog.h"

extern "C" {

#include "vim.h"
#include "globals.h"

static QVimShell *vimshell = NULL;
static MainWindow *window = NULL;

static QColor foregroundColor;
static QColor backgroundColor;
static QColor specialColor;

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
 */
GuiFont
gui_mch_get_font(char_u *name, int giveErrorIfMissing)
{
	QString family = (char*)name;
	QFont font;
	font.setStyleHint(QFont::TypeWriter);

	if ( name == NULL ) { // Fallback font
		font.setFamily("Monospace");
		font.setPointSize(10);
		font.setKerning(false);
		return new QFont(font);
	}

	bool ok;
	int size = family.section(' ', -1).trimmed().toInt(&ok);
	if ( ok ) {
		QString realname = family.section(' ', 0, -2).trimmed();
		font.setFamily(realname);
		font.setPointSize(size);
	} else if ( !font.fromString((char*)name) ) {
		font.setRawName((char*)name);
	}

	// I expected QFont::exactMatch to do this - but I was wrong
	// FIXME: this needs further testing
    // This makes it impossible to set the font to "Monospace".
	if ( QFontInfo(font).family() != font.family() && giveErrorIfMissing ) {
        QString errmsg;
        QTextStream(&errmsg) << "Font called "
            << font.family() << " is actually called "
            << QFontInfo(font).family();
        
        EMSG2(e_font, errmsg.toUtf8().data());
		return NOFONT;
	}

	font.setFixedPitch(true);
	font.setBold(false);
	font.setItalic(false);
	font.setKerning(false);

	return new QFont(font);
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
    if (!vim_is_input_buf_empty())
        return OK;
    
	if ( wtime == -1 ) {
		QApplication::processEvents( QEventLoop::WaitForMoreEvents);
		return OK;
	} else {
		// @see gui_mch_update
		QTime t;
		t.start();
		do {
			QApplication::processEvents( QEventLoop::WaitForMoreEvents, wtime );
			if (!vim_is_input_buf_empty()) {
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
	if ( QApplication::hasPendingEvents() ) {
		QApplication::processEvents();
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
	if ( color != INVALCOLOR ) {
		foregroundColor = VimWrapper::fromColor(color);
	}
}

/**
 * Set the current text background color.
 */
void
gui_mch_set_bg_color(guicolor_T color)
{
	// The shell needs a hint background color
	// to paint the back when resizing
	if ( color != INVALCOLOR ) {
		backgroundColor = VimWrapper::fromColor(color);
	}
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
	op.color = VimWrapper::fromColor(gui.back_pixel);
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
	QFont *qf = gui_mch_get_font(font_name, TRUE);
	if ( qf == NULL ) {
		return FAIL;
	}

	QFontMetrics metric( *qf );

	if ( metric.averageCharWidth() != metric.maxWidth() ) {
		qDebug() << "Warning, fake monospace font?";
	}

	gui.norm_font = qf;
	gui.char_width = metric.width("M");
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
	return (long_u)pixel;
}

/**
 * Clear the block with the given coordinates
 */
void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
	QRect rect = VimWrapper::mapBlock(row1, col1, row2, col2); 

	PaintOperation op;
	op.type = FILLRECT;
	op.color = VimWrapper::fromColor(gui.back_pixel);
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
	op.color = VimWrapper::fromColor(gui.back_pixel);
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
	QRect scrollRect = VimWrapper::mapBlock(row, gui.scroll_region_left, 
					gui.scroll_region_bot, gui.scroll_region_right);

	PaintOperation op1;
	op1.type = SCROLLRECT;
	op1.rect = scrollRect;
	op1.pos = QPoint(0, num_lines*gui.char_height);
	op1.color = VimWrapper::fromColor(gui.back_pixel);
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
	QRect scrollRect = VimWrapper::mapBlock(row, gui.scroll_region_left, 
					gui.scroll_region_bot, gui.scroll_region_right);

	PaintOperation op;
	op.type = SCROLLRECT;
	op.rect = scrollRect;
	op.pos = QPoint(0, -num_lines*gui.char_height);
	op.color = VimWrapper::fromColor(gui.back_pixel);
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

	window = new MainWindow(&gui);

	// Load qVim settings
	QSettings settings("Vim", "qVim");
	settings.beginGroup("mainwindow");
	window->restoreState( settings.value("state").toByteArray() );
	window->resize( settings.value("size", QSize(400, 400)).toSize() );
	settings.endGroup();

	vimshell = window->vimShell();

	// Load qVim style
	QSettings ini(QSettings::IniFormat, QSettings::UserScope, "Vim", "qVim");
	QFile styleFile( QFileInfo(ini.fileName()).absoluteDir().absoluteFilePath("qVim.style") );

	if ( styleFile.open(QIODevice::ReadOnly) ) {
		window->setStyleSheet( styleFile.readAll() );
		styleFile.close();
	}

	// Clipboard - the order matters, for safety
	clip_plus.clipboardMode = QClipboard::Selection;
	clip_star.clipboardMode = QClipboard::Clipboard;

	QObject::connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)),
			vimshell, SLOT(clipboardChanged(QClipboard::Mode)));


	display_errors();

	/* Colors */
	gui.norm_pixel = VimWrapper::toColor(QColor(Qt::black));
	gui.back_pixel = VimWrapper::toColor(QColor(Qt::white));

	set_normal_colors();
	gui_check_colors();
	highlight_gui_started();

	gui.def_norm_pixel = VimWrapper::toColor(QColor(Qt::black));
	gui.def_back_pixel = VimWrapper::toColor(QColor(Qt::white));

	// The Scrollbar manages the scrollbars
	gui.scrollbar_width = 0;
	gui.scrollbar_height = 0;

	// Background color hint
	vimshell->setBackground(VimWrapper::backgroundColor() );

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
#ifdef Q_WS_X11
	bool useGUI = getenv("DISPLAY") != 0;
#else
	bool useGUI = true;
#endif
	QApplication *app = new QApplication(*argc, argv, useGUI);


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
}

/**
 * Called when the foreground or background color has been changed.
 */
void
gui_mch_new_colors()
{
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
		window->setWindowTitle( VimWrapper::convertFrom((char*)title) );
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
	QRect rect = VimWrapper::mapBlock(row, col, row+nr-1, col +nc-1);

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
 */
int
gui_mch_init_check()
{
#ifdef Q_WS_X11
	return (getenv("DISPLAY") != 0);
#else
	return OK;
#endif
}

/* Clipboard */

/**
 * Own the selection and return OK if it worked.
 * 
 */
int
clip_mch_own_selection(VimClipboard *cbd)
{
	return OK;
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
		clip->setText( VimWrapper::convertFrom((char *)str, size), (QClipboard::Mode)cbd->clipboardMode);
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
	QByteArray text = VimWrapper::convertTo(clip->text( (QClipboard::Mode)cbd->clipboardMode));

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
	QPoint br = QPoint(FILL_X(gui.col)+w-2, 
			FILL_Y(gui.row)+gui.char_height-2);
	QRect rect(tl, br);

	PaintOperation op;
	op.type = DRAWRECT;
	op.rect = rect;
	op.color = VimWrapper::fromColor(color);
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
	op.color = VimWrapper::fromColor(color);
	vimshell->queuePaintOp(op);
}

/**
 * Set the current text special color.
 */
void
gui_mch_set_sp_color(guicolor_T color) 
{
	if ( color != INVALCOLOR ) {
		specialColor = VimWrapper::fromColor(color);
	}
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
	QString str = VimWrapper::convertFrom((char *)s, len);
	
	// Font
	QFont f = vimshell->font();
	f.setBold( flags & DRAW_BOLD);
	f.setUnderline( flags & DRAW_UNDERL);
	f.setItalic( flags & DRAW_ITALIC);

	QFontMetrics fm(f);

	int cellwidth = VimWrapper::stringCellWidth(str);
	QPoint pos = VimWrapper::mapText(row, col);
	QRect rect( pos.x(), pos.y(), gui.char_width*cellwidth, gui.char_height);

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
		return VimWrapper::toColor(c);
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
	QAction *before=NULL;

	if ( menu_is_popup(menu->name) ) {
		menu->qmenu = new QMenu(VimWrapper::convertFrom((char*)menu->name), vimshell);
		return;
	} else if ( menu_is_toolbar(menu->name) ) {
		menu->qmenu = window->toolBar();
	} else if ( menu->parent == NULL ) {
		QList<QAction*> actions = window->menuBar()->actions();
		if ( idx < actions.size() ) {
			before = actions.at(idx);
		}

		QMenu *m = new QMenu(VimWrapper::convertFrom((char*)menu->name), window);
		window->menuBar()->insertMenu( before, m);
		menu->qmenu = m;
	} else if ( menu->parent && menu->parent->qmenu ) {
		QMenu *parent = (QMenu*)menu->parent->qmenu;
		QList<QAction*> actions = parent->actions();
		if ( idx < actions.size() ) {
			before = actions.at(idx);
		}

		QMenu *m = new QMenu(VimWrapper::convertFrom((char*)menu->name), window);
		parent->insertMenu( before, m);
		menu->qmenu = m;
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

	QList<QAction*> actions = menu->parent->qmenu->actions();
	QAction *before=NULL;
	if ( idx < actions.size() ) {
		before = actions.at(idx);
	}

	if ( menu_is_toolbar(menu->parent->name) ) {
		// Toolbar
		QToolBar *b = (QToolBar*)menu->parent->qmenu;
		if (menu_is_separator(menu->name)) {
			b->addSeparator();
		} else {
			QAction *action = new VimAction( menu, window );
			b->insertAction(before, action);
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
			m->insertAction(before, action);
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
	if ( menu == NULL || menu->qmenu == NULL ) {
		return;
	}

	QMenu *m = (QMenu*)menu->qmenu;
	m->exec( QCursor::pos() );
}

/**
 * Set the menu and scrollbar colors to their default values.
 */
void
gui_mch_def_colors()
{
	gui.norm_pixel = VimWrapper::toColor(QColor(Qt::black));
	gui.back_pixel = VimWrapper::toColor(QColor(Qt::white));
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

	sb->wid->setEnabled( !(size > max) );
}

/**
 * Set scrollbar geometry
 */
void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int width, int height)
{
	switch(sb->type) {
	case SBAR_RIGHT:
	case SBAR_LEFT:
		sb->wid->setLength(height);
		sb->wid->setIndex(y);
		break;
	default:
		sb->wid->setLength(width);
		sb->wid->setIndex(x);
	}
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

	VimScrollBar *widget = new VimScrollBar( sb, dir, window);
	widget->setVisible(false);
	sb->wid = widget;

	switch(sb->type) {
	case SBAR_RIGHT:
		window->addScrollbarRight(widget);
		break;
	case SBAR_LEFT:
		window->addScrollbarLeft(widget);
		break;
	case SBAR_BOTTOM:
		window->addScrollbarBottom(widget);
		break;
	}
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

	return vim_strsave((char_u *) VimWrapper::convertTo(file).data());
}


/**
 * Open a dialog window
 */
int
gui_mch_dialog(int type, char_u *title, char_u *message, char_u *buttons, int dfltbutton, char_u *textfield, int ex_cmd)
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

		b_string = VimWrapper::convertFrom((char*)buttons).split(DLG_BUTTON_SEP);

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
		window->setTab( nr, VimWrapper::convertFrom((char*)labeltext));
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
	QFont *oldfont = gui_mch_get_font(oldval, 0);

	bool ok;
	static FontDialog *dialog = new FontDialog();
	if ( oldfont != NULL ) {
		dialog->selectCurrentFont(*oldfont);
	}

	if ( dialog->exec() == QDialog::Accepted ) {
		QFont f =  dialog->selectedFont();
		QByteArray text = VimWrapper::convertTo( QString("%1 %2").arg(f.family()).arg(f.pointSize()) );

		char_u *buffer;
		buffer = lalloc( text.size(), TRUE);
		for (int i = 0; i < text.size(); ++i) {
			buffer[i] = text[i];
		}
		return buffer;
	}

	return NULL;
}

} // extern "C"
