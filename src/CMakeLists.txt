# Source files
SET(BEQUNGE-SOURCES
	main.cpp
	mainwindow.cpp
	interpreter.cpp
	fungespace.cpp
	glview.cpp
	OGLFT.cpp
	stringmodewidget.cpp
	fungeconsole.cpp
	fungecommand.cpp
	stackstack.cpp
	coord.cpp
	openal.cpp)

# Header files that have Q_OBJECT in
SET(BEQUNGE-MOC-HEADERS
	fungeconsole.h
	fungespace.h
	glview.h
	interpreter.h
	mainwindow.h
	stringmodewidget.h
	openal.h)

# UI files
SET(BEQUNGE-UI
	mainwindow.ui)

# Resource files
SET(BEQUNGE-RESOURCES
	resources.qrc)


# Magic

SET(CMAKE_CXX_FLAGS ${FREETYPE_CFLAGS})

QT4_WRAP_CPP(BEQUNGE-SOURCES-MOC ${BEQUNGE-MOC-HEADERS})
QT4_WRAP_UI(BEQUNGE-SOURCES-UI ${BEQUNGE-UI})
QT4_ADD_RESOURCES(BEQUNGE-SOURCES-RESOURCE ${BEQUNGE-RESOURCES})

ADD_EXECUTABLE(bequnge ${BEQUNGE-SOURCES} ${BEQUNGE-SOURCES-MOC} ${BEQUNGE-SOURCES-UI} ${BEQUNGE-SOURCES-RESOURCE})

TARGET_LINK_LIBRARIES(bequnge ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTOPENGL_LIBRARY} ${FREETYPE_LINK_FLAGS} ${OPENAL_LIBRARY} "-lalut" ${OGGVORBIS_LIBRARIES})