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

        ../movie/OverlayMaker.cpp
        ../movie/PolarOverlayMaker.cpp
        ../movie/RudderOverlayMaker.cpp
        ../movie/TargetsOverlayMaker.cpp

        test_polars.cpp
        test_median.cpp
        test_overlays.cpp
        )

target_include_directories(TestPolars PUBLIC
        ../libInterpolate/src
        ..
        ../gui
)

target_link_libraries(TestPolars
        PRIVATE
        GTest::GTest
        Qt::Gui
        Qt::Qml

        )

add_test (NAME MyTest COMMAND TestPolars WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
