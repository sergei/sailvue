cmake_minimum_required(VERSION 3.25)
project(sailvue)

set(CMAKE_CXX_STANDARD 17)

add_executable(sailvue main.cpp
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
        n2k/InitCanBoat.c n2k/InitCanBoat.h n2k/geo/Quantity.h n2k/geo/Angle.h n2k/InstrumentInput.h n2k/geo/Quantity.cpp n2k/geo/Direction.h n2k/geo/Speed.h n2k/geo/UtcTime.h
        n2k/geo/GeoLoc.h)

target_include_directories(sailvue PUBLIC
        n2k/canboat/analyzer
        n2k/canboat/common
        )
