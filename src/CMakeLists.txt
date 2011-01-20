cmake_minimum_required(VERSION 2.8)

include_directories(proto auto .)

set(VIM_SOURCES buffer.c blowfish.c charset.c diff.c digraph.c edit.c eval.c ex_cmds.c ex_cmds2.c ex_docmd.c ex_eval.c ex_getln.c fileio.c fold.c getchar.c hardcopy.c hashtab.c  if_cscope.c if_xcmdsrv.c main.c mark.c memfile.c memline.c menu.c message.c misc1.c misc2.c move.c mbyte.c normal.c ops.c option.c os_unix.c popupmnu.c quickfix.c regexp.c screen.c search.c sha256.c spell.c syntax.c  tag.c term.c ui.c undo.c window.c gui.c  netbeans.c pty.c version.c auto/pathdef.c)

#set(GUI_SOURCES gui_athena.c gui_x11.c pty.c gui_beval.c gui_at_sb.c gui_at_fs.c)
set(GUI_SOURCES gui_qt.cpp)

#add_definitions(-DHAVE_CONFIG_H -DFEAT_GUI_ATHENA)
add_definitions(-DHAVE_CONFIG_H -DFEAT_GUI -DNO_X11_INCLUDES -DFEAT_GUI_QT -UHAVE_X11 -UFEAT_MENU)


add_executable(vim ${VIM_SOURCES} ${GUI_SOURCES})
target_link_libraries(vim -lm -lncurses -lacl -lgpm -lQtGui)
