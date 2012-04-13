cmake_minimum_required(VERSION 2.8)

find_package(Qt4 REQUIRED)
set( QT_USE_QTGUI TRUE )
include(${QT_USE_FILE} )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(proto auto . qt)

# pathdef
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pathdef.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/auto/pathdef.c)

#
# Vim executable
#

set(VIM_SOURCES buffer.c 
		blowfish.c 
		charset.c 
		diff.c 
		digraph.c 
		edit.c 
		eval.c 
		ex_cmds.c 
		ex_cmds2.c 
		ex_docmd.c 
		ex_eval.c 
		ex_getln.c 
		fileio.c 
		fold.c 
		getchar.c 
		hardcopy.c 
		hashtab.c 
		if_cscope.c 
		if_xcmdsrv.c 
		main.c 
		mark.c 
		memfile.c 
		memline.c 
		menu.c 
		message.c 
		misc1.c 
		misc2.c 
		move.c 
		mbyte.c 
		normal.c 
		ops.c 
		option.c 
		popupmnu.c 
		quickfix.c 
		regexp.c 
		screen.c 
		search.c 
		sha256.c 
		spell.c 
		syntax.c 
		tag.c 
		term.c 
		ui.c 
		undo.c 
		window.c 
		gui.c 
		netbeans.c 
		version.c 
		auto/pathdef.c)

set(GUI_SOURCES gui_qt.cpp )
set(MOC_SOURCES qt/qvimshell.cpp 
		qt/mainwindow.cpp 
		qt/vimaction.cpp 
		qt/tabbar.cpp 
		qt/vimaction.cpp 
		qt/vimscrollbar.cpp 
		qt/vimwrapper.cpp 
		qt/fontdialog.cpp
		qt/scrollarea.cpp)

set(QVIM_RCS qt/icons.qrc)
QT4_ADD_RESOURCES( QVIM_RC_SRCS ${QVIM_RCS} )

qt4_automoc(${MOC_SOURCES})


if(MSVC OR MINGW)

	set(VIM_SOURCES ${VIM_SOURCES} 
			os_win32.c 
			os_mswin.c)

	add_definitions(-DWIN32
			-DNO_X11_INCLUDES
			-DFEAT_GUI_QT
			-UHAVE_X11
			-DFEAT_HUGE
			-DFEAT_MBYTE
			-UFEAT_SIGN
			-D__func__=__FUNCTION__
			-D_CRT_SECURE_NO_WARNINGS)

	if (MSVC)
		add_definitions(-wd4099)
	endif (MSVC)

	add_executable(qvim WIN32 ${VIM_SOURCES} ${GUI_SOURCES} ${MOC_SOURCES} ${QVIM_RC_SRCS})

	target_link_libraries(qvim ${QT_QTMAIN_LIBRARY} # Link to qtmain.lib to avoid WinMain()
				   ${QT_LIBRARIES} )	# Link to Qt
else(MSVC OR MINGW)
	message(FATAL_ERROR "Could not find Visual Studio or Mingw - are they on the path? This is Windows, right?")
endif(MSVC OR MINGW)


