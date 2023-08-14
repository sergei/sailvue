cmake_minimum_required(VERSION 3.25)
project(sailvue)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Positioning)
find_package(Qt6 COMPONENTS Location)
find_package(Qt6 REQUIRED COMPONENTS Multimedia)

qt_add_executable(sailvue WIN32 MACOSX_BUNDLE
        main.cpp
        InstrDataReader.h

        n2k/canboat/analyzer/pgn.c
        n2k/canboat/analyzer/pgn.h
        n2k/canboat/analyzer/lookup.c
        n2k/canboat/analyzer/lookup.h
        n2k/canboat/analyzer/print.c
        n2k/canboat/analyzer/fieldtype.c
        n2k/canboat/analyzer/fieldtype.h
        n2k/canboat/common/common.c
        n2k/canboat/common/utf.c
        n2k/YdvrReader.cpp
        n2k/YdvrReader.h
        n2k/InitCanBoat.c
        n2k/InitCanBoat.h
        n2k/geo/Quantity.h
        n2k/geo/Angle.h
        n2k/geo/Quantity.cpp
        n2k/geo/Direction.h
        n2k/geo/Speed.h
        n2k/geo/UtcTime.h
        n2k/geo/GeoLoc.h

        gopro/GoPro.cpp
        gopro/GoPro.h
        gopro/gpmf-parser/GPMF_parser.c
        gopro/gpmf-parser/demo/GPMF_mp4reader.c

        navcomputer/InstrumentInput.h
        navcomputer/IProgressListener.h
        navcomputer/RaceData.cpp
        navcomputer/RaceData.h
        navcomputer/Chapter.cpp
        navcomputer/Chapter.h

        gui/Worker.cpp
        gui/Worker.h
        gui/Project.cpp
        gui/Project.h
        movie/MovieProducer.cpp
        movie/MovieProducer.h
        movie/InstrOverlayMaker.cpp
        movie/InstrOverlayMaker.h
        ffmpeg/FFMpeg.cpp
        ffmpeg/FFMpeg.h
)

target_include_directories(sailvue PUBLIC
        n2k/canboat/analyzer
        n2k/canboat/common
        cxxopts/include
        gopro/gpmf-parser
        gopro/gpmf-parser/demo
        gui
        )

target_link_libraries(sailvue PUBLIC
        Qt::Core
        Qt::Gui
        Qt::LocationPrivate
        Qt::Positioning
        Qt::Qml
        Qt::Quick
        Qt6::Multimedia
)

# Resources:
set(qml_resource_files
        "gui/main.qml"
        "gui/RaceEditor.qml"
        "gui/ChapterEditor.qml"
        "gui/ChapterMapElement.qml"
        "gui/InputDataDialog.qml"
)

qt6_add_resources(sailvue "qml"
        PREFIX
        "/"
        FILES
        ${qml_resource_files}
)

qt_add_resources(sailvue "app_images"
        PREFIX "/"
        FILES
        "images/mapmarker.png"
)

# Suppress policy warning
qt_policy(
        SET QTP0001 NEW
)

qt_add_qml_module(sailvue
        URI sails
        VERSION 1.0
        SOURCES gui/RaceTreeModel.h
        SOURCES gui/RaceTreeModel.cpp
        SOURCES gui/GoProMediaPlayer.h
        SOURCES gui/GoProMediaPlayer.cpp
)

