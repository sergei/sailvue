include(FetchContent)

find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

add_executable (TestPolars
        ../navcomputer/polars.h
        ../navcomputer/polars.cpp
        ../navcomputer/Chapter.cpp

        ../n2k/geo/Quantity.cpp
        ../n2k/YdvrReader.cpp
        ../n2k/canboat/analyzer/pgn.c
        ../n2k/canboat/analyzer/pgn.h
        ../n2k/canboat/analyzer/lookup.c
        ../n2k/canboat/analyzer/lookup.h
        ../n2k/canboat/analyzer/print.c
        ../n2k/canboat/analyzer/fieldtype.c
        ../n2k/canboat/analyzer/fieldtype.h
        ../n2k/canboat/common/common.c
        ../n2k/canboat/common/utf.c
        ../n2k/YdvrReader.cpp
        ../n2k/YdvrReader.h
        ../n2k/InitCanBoat.c
        ../n2k/InitCanBoat.h

        ../movie/OverlayMaker.cpp
        ../movie/PolarOverlayMaker.cpp
        ../movie/RudderOverlayMaker.cpp
        ../movie/TargetsOverlayMaker.cpp
        ../movie/InstrOverlayMaker.cpp
        ../movie/PerformanceOverlayMaker.cpp
        ../adobe_premiere/MarkerReader.cpp
        ../Insta360/Insta360.cpp
        ../cameras/CameraBase.cpp

        test_polars.cpp
        test_median.cpp
        test_overlays.cpp
        test_adobe_markers.cpp
        test_n2k.cpp
        )

target_include_directories(TestPolars PUBLIC
        ../libInterpolate/src
        ..
        ../gui
        ../n2k/canboat/analyzer
        ../n2k/canboat/common
)

target_link_libraries(TestPolars
        PRIVATE
        GTest::GTest
        Qt::Gui
        Qt::Qml

        )

add_test (NAME MyTest COMMAND TestPolars WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
