include_directories(${crypto_SOURCE_DIR}/SatELite/ForMani/ADTs)
include_directories(${crypto_SOURCE_DIR}/SatELite/ForMani/Global)
include_directories(${crypto_SOURCE_DIR}/SatELite/SatELite)

add_executable(satelite
    SatELite/Solver.C
    SatELite/Solver_subsume.C
    SatELite/TmpFiles.C
    SatELite/Main.C
    SatELite/Main_debug.C
    SatELite/Profile.C
    ForMani/ADTs/File.C
    ForMani/Global/Global.C
    ForMani/Global/String.C
) 

target_link_libraries(satelite
    ${ZLIB_LIBRARY}
)

