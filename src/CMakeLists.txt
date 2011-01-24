cmake_minimum_required(VERSION 2.8)

find_package(Qt4 REQUIRED)
set( QT_USE_QTGUI TRUE )
include(${QT_USE_FILE} )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(proto auto .)



set(VIM_SOURCES buffer.c blowfish.c charset.c diff.c digraph.c edit.c eval.c ex_cmds.c ex_cmds2.c ex_docmd.c ex_eval.c ex_getln.c fileio.c fold.c getchar.c hardcopy.c hashtab.c  if_cscope.c if_xcmdsrv.c main.c mark.c memfile.c memline.c menu.c message.c misc1.c misc2.c move.c mbyte.c normal.c ops.c option.c os_unix.c popupmnu.c quickfix.c regexp.c screen.c search.c sha256.c spell.c syntax.c  tag.c term.c ui.c undo.c window.c gui.c  netbeans.c pty.c version.c auto/pathdef.c)

set(GUI_SOURCES gui_qt.cpp)
set(MOC_SOURCES qvimshell.cpp)

qt4_automoc(${MOC_SOURCES})

add_definitions(-DHAVE_CONFIG_H -DFEAT_GUI -DNO_X11_INCLUDES -DFEAT_GUI_QT -UHAVE_X11 -UFEAT_MENU -DFEAT_BROWSE -DFEAT_GUI_DIALOG -DFEAT_MBYTE)
add_executable(vim ${VIM_SOURCES} ${GUI_SOURCES} ${MOC_SOURCES})
target_link_libraries(vim -lm -lncurses -lacl -lgpm ${QT_LIBRARIES})
