cmake_minimum_required(VERSION 3.16)
project(artes-papeis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt modules
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set Qt root dir
set(QT_ROOT "/Users/valass/Qt/6.10.0/macos")

# Allow Qt directory to be specified through command line or environment
if(DEFINED ENV{Qt6_DIR} AND NOT Qt6_DIR)
    set(Qt6_DIR $ENV{Qt6_DIR})
else()
    set(Qt6_DIR "${QT_ROOT}/lib/cmake/Qt6")
endif()

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
    Network
)

# Set include path explicitly
include_directories("${QT_ROOT}/include")
include_directories("${QT_ROOT}/include/QtWidgets")
include_directories("${QT_ROOT}/lib/QtWidgets.framework/Headers")

# Add Qt module include paths
include_directories(${Qt6_DIR}/../../../include)
include_directories(${Qt6_DIR}/../../../include/QtWidgets)
include_directories(${Qt6Core_INCLUDE_DIRS})
include_directories(${Qt6Gui_INCLUDE_DIRS})
include_directories(${Qt6Widgets_INCLUDE_DIRS})

# Add executable
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    clickablelabel.h
    productcard.cpp
    productcard.h
    productformdialog.cpp
    productformdialog.h
    productmanager.cpp
    productmanager.h
    produto.h
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Core_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Gui_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Widgets_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6_DIR}/../../../include/QtWidgets)

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)