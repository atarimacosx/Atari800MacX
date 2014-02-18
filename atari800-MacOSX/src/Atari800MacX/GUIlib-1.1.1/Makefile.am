# Makefile.am for the SDL sample image loading library and viewer

lib_LTLIBRARIES = libGUI.la

libGUIincludedir = $(includedir)/GUI
libGUIinclude_HEADERS =		\
	GUI.h			\
	GUI_C.h			\
	GUI_status.h		\
	GUI_widget.h		\
	GUI_widgets.h		\
	GUI_area.h		\
	GUI_button.h		\
	GUI_generic.h		\
	GUI_image.h		\
	GUI_scroll.h		\
	GUI_scrollbar.h		\
	GUI_termwin.h		\
	GUI_font.h		\
	GUI_menu.h

libGUI_la_SOURCES =		\
	GUI.cpp			\
	GUI_C.cpp		\
	GUI_area.cpp		\
	GUI_button.cpp		\
	GUI_font.cpp		\
	GUI_generic.cpp		\
	GUI_image.cpp		\
	GUI_loadimage.cpp	\
	GUI_loadimage.h		\
	GUI_menu.cpp		\
	GUI_output.cpp		\
	GUI_output.h		\
	GUI_scrollbar.cpp	\
	GUI_termwin.cpp		\
	GUI_types.h		\
	GUI_widget.cpp		\
	cancel_img.h		\
	okay_img.h		\
	the_font.h		\
	the_checker.h

EXTRA_DIST =			\
	CHANGES			\
	autogen.sh		\
	MPWmake.sea.hqx		\
	hello.bmp		\
	hello2.bmp		\
	scroll_dn.bmp		\
	scroll_up.bmp
libGUI_la_LDFLAGS = 		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)


noinst_PROGRAMS = genimage hello hello_C keyboard okay

genimage_SOURCES = genimage.c
hello_SOURCES = hello.cpp
hello_LDADD = libGUI.la
hello_C_SOURCES = hello_C.c
hello_C_LDADD = libGUI.la
keyboard_SOURCES = keyboard.cpp
keyboard_LDADD = libGUI.la
okay_SOURCES = okay.c
okay_LDADD = libGUI.la
