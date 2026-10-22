cmake_minimum_required(VERSION 3.16)

# Tracy profiler integration
set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.14.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tracy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Enable C++14 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # Turn on warnings

# Qt compilers setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE_GUI_VIEWER
	AnimationHandler.cpp
	colorManager.cpp
	imageCache.cpp
	imageViewer.cpp
	qrect_extras.cpp
	viewboxHandler.cpp
	ZoomBox.cpp
	)

# Make sure Windows is set to use the unicode API's
if( WIN32 )
	add_definitions(-DUNICODE -D_UNICODE)
endif( WIN32 )

# Set-up libraries
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Concurrent)

add_library(qtimgviewer ${SOURCE_GUI_VIEWER} )
target_link_libraries(qtimgviewer PRIVATE Qt6::Gui Qt6::Widgets Qt6::Concurrent -llcms2 ${PLATFORM_LIBRARIES})

if(ENABLE_TRACY)
    target_link_libraries(qtimgviewer PUBLIC TracyClient)
endif()

# Enable C++17 features
set_property(TARGET qtimgviewer PROPERTY CXX_STANDARD 17)
set_property(TARGET qtimgviewer PROPERTY CXX_STANDARD_REQUIRED ON)
