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

# For profiling
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0    -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0   -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")

find_package(Boost )   # Required for  libInterpolate
if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
endif()

set(N2K_SRC
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
)

set(NMEA2000_SRC
        n2k/NMEA2000/src/NMEA2000.cpp
        n2k/NMEA2000/src/N2kGroupFunction.cpp
        n2k/NMEA2000/src/N2kGroupFunctionDefaultHandlers.cpp
        n2k/NMEA2000/src/N2kMsg.cpp
        n2k/NMEA2000/src/N2kStream.cpp
        n2k/NMEA2000/src/N2kMessages.cpp
        n2k/NMEA2000/src/N2kTimer.cpp
)

set(GOPRO_SRC
        gopro/GoPro.cpp
        gopro/GoPro.h
        gopro/gpmf-parser/GPMF_parser.c
        gopro/gpmf-parser/demo/GPMF_mp4reader.c
)

set(GUI_SRC
        gui/Worker.cpp
        gui/Worker.h
        gui/Project.cpp
        gui/Project.h
        gui/ChapterMaker.cpp
        gui/Settings.cpp
        gui/Settings.h
)

set(MOVIE_SRC
        movie/MovieProducer.cpp
        movie/MovieProducer.h
        movie/InstrOverlayMaker.cpp
        movie/InstrOverlayMaker.h
        movie/PolarOverlayMaker.cpp
        movie/PolarOverlayMaker.h
        movie/StartTimerOverlayMaker.cpp
        movie/StartTimerOverlayMaker.h
        movie/TargetsOverlayMaker.cpp
        movie/TargetsOverlayMaker.h
        movie/PerformanceOverlayMaker.cpp
        movie/PerformanceOverlayMaker.h
)

set(FFMPEG_SRC
        ffmpeg/FFMpeg.cpp
        ffmpeg/FFMpeg.h
)

set (NAV_SRC
        navcomputer/InstrumentInput.h
        navcomputer/IProgressListener.h
        navcomputer/RaceData.cpp
        navcomputer/RaceData.h
        navcomputer/Chapter.cpp
        navcomputer/Chapter.h
        navcomputer/NavStatsEventsListener.h
        navcomputer/NavStats.cpp
        navcomputer/NavStats.h
        navcomputer/Polars.cpp
        navcomputer/Polars.h
        navcomputer/TimeDeltaComputer.cpp
        navcomputer/TimeDeltaComputer.h
        navcomputer/Performance.h
        navcomputer/Calibration.h
        navcomputer/Calibration.cpp
        movie/OverlayElement.cpp
        movie/OverlayElement.h
        movie/OverlayMaker.cpp
        movie/OverlayMaker.h
        net_sim/NetworkSimulator.cpp
        net_sim/NetworkSimulator.h
        net_sim/NMEA2000_UDP.cpp
        net_sim/NMEA2000_UDP.h
        net_sim/N2kDebugStream.cpp
        net_sim/N2kDebugStream.h
)

qt_add_executable(sailvue WIN32 MACOSX_BUNDLE
        main.cpp

        ${N2K_SRC}
        ${NMEA2000_SRC}
        ${GOPRO_SRC}
        ${GUI_SRC}
        ${MOVIE_SRC}
        ${FFMPEG_SRC}
        ${NAV_SRC}

        utils/Caffeine.cpp
        utils/Caffeine.h
)

target_include_directories(sailvue PUBLIC
        n2k/canboat/analyzer
        n2k/canboat/common
        cxxopts/include
        gopro/gpmf-parser
        gopro/gpmf-parser/demo
        gui
        libInterpolate/src
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

enable_testing ()
add_subdirectory (test)

# Resources:
set(qml_resource_files
        "gui/main.qml"
        "gui/RaceEditor.qml"
        "gui/ChapterEditor.qml"
        "gui/ChapterMapElement.qml"
        "gui/InputDataDialog.qml"
        "gui/PGNSources.qml"
        "gui/Options.qml"
        "gui/RaceVideo.qml"
        "gui/RaceMap.qml"
        "gui/RaceTreeView.qml"
        "gui/RaceTimeLine.qml"
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
        "images/icons8-zoom-in-50.png"
        "images/icons8-zoom-out-50.png"
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
        SOURCES gui/PgnSrcTreeModel.h
        SOURCES gui/PgnSrcTreeModel.cpp
        SOURCES gui/ChapterTypes.h
)

add_executable(ydn2csv
        ydn2csv/main.cpp

        ${N2K_SRC}
)

target_include_directories(ydn2csv PUBLIC
        n2k/canboat/analyzer
        n2k/canboat/common
        cxxopts/include
)
