cmake_minimum_required(VERSION 3.0)
project(imgviewer)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(viewer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

# Qt compilers setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_CORE
	fileManager.cpp
	imageContainer.cpp
	imageLoader.cpp
	meta.cpp
	windowManager.cpp
	)

set(SOURCE_IMAGE_READER
	ImageReader/AnimCombiner.cpp
	ImageReader/ImageReader.cpp
	ImageReader/ReaderGif.cpp
	ImageReader/ReaderJpeg.cpp
	ImageReader/ReaderPng.cpp
	ImageReader/ReaderQt.cpp
	)

set(SOURCE_FILE_SYSTEM
	FileSystem/ExtensionChecker.cpp
	)

set(RESOURCES
	resources/resources.qrc
	)
if( WIN32 ) # Windows executable icon
	set(RESOURCES ${RESOURCES} resources/icon.rc)
endif( WIN32 )

if( WIN32 ) # Don't use registry on windows for saving options
	add_definitions( -DPORTABLE )
endif( WIN32 )

add_executable(imgviewer WIN32 ${SOURCE_CORE} ${SOURCE_IMAGE_READER} ${SOURCE_FILE_SYSTEM} ${extra_moc} ${RESOURCES} main.cpp)

# Set-up libraries
find_package(Qt5Widgets REQUIRED)

target_link_libraries(imgviewer qtimgviewer Qt5::Widgets -lexif -lpng -lz -ljpeg -lgif)

install(FILES resources/imgviewer.desktop DESTINATION /usr/share/applications) # TODO: This will probably fail on other systems/distributions
install(TARGETS imgviewer RUNTIME DESTINATION bin)

# Enable C++14 features
set_property(TARGET imgviewer PROPERTY CXX_STANDARD 14)
set_property(TARGET imgviewer PROPERTY CXX_STANDARD_REQUIRED ON)
