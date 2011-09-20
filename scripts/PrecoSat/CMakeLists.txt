include_directories(${cryptoms_SOURCE_DIR}/scripts/PrecoSat/)

add_definitions(-DNLOGPRECO)

add_executable(precosat
    precobnr.cc
    precosat.cc
    precomain.cc
    )
