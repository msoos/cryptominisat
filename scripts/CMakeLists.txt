add_custom_target(copy ALL
    COMMENT "Copying CryptoMiniSat")

add_custom_command(
    TARGET copy
    COMMAND ${CMAKE_COMMAND} -E copy ../Solver/cryptominisat ../
    COMMAND ${CMAKE_COMMAND} -E copy ../SatElite/satelite ../
)

