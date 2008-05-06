INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../src")

SET(FUNGESPACE-SOURCES
	testfungespace.cpp
	../src/fungespace.cpp
	../src/coord.cpp
)
SET(FUNGESPACE-MOC-HEADERS
	testfungespace.h
	../src/fungespace.h
)
QT4_WRAP_CPP(FUNGESPACE-SOURCES-MOC ${FUNGESPACE-MOC-HEADERS})
ADD_EXECUTABLE(testfungespace ${FUNGESPACE-SOURCES} ${FUNGESPACE-SOURCES-MOC})
TARGET_LINK_LIBRARIES(testfungespace ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY})

SET(COORD-SOURCES
	testcoord.cpp
	../src/coord.cpp
)
SET(COORD-MOC-HEADERS
	testcoord.h
)
QT4_WRAP_CPP(COORD-SOURCES-MOC ${COORD-MOC-HEADERS})
ADD_EXECUTABLE(testcoord ${COORD-SOURCES} ${COORD-SOURCES-MOC})
TARGET_LINK_LIBRARIES(testcoord ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY})

ADD_CUSTOM_TARGET(test
	./testcoord
	COMMAND ./testfungespace
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
	DEPENDS testcoord testfungespace)



SET (EXECUTABLE_OUTPUT_PATH ..)