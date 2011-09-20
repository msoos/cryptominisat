include_directories(${cryptoms_SOURCE_DIR}/dsharp/shared)
include_directories(${cryptoms_SOURCE_DIR}/dsharp/src_sharpSAT)

add_subdirectory(dnnf2cnf)

add_executable(dsharp_exec
    shared/SomeTime.cpp
    shared/RealNumberTypes.cpp
    shared/Interface/AnalyzerData.cpp
    src_sharpSAT/Basics.cpp
    src_sharpSAT/MainSolver/DecisionStack.cpp
    src_sharpSAT/MainSolver/DecisionTree.cpp
    src_sharpSAT/MainSolver/MainSolver.cpp
    src_sharpSAT/MainSolver/InstanceGraph/InstanceGraph.cpp
    src_sharpSAT/MainSolver/InstanceGraph/AtomsAndNodes.cpp
    src_sharpSAT/main.cpp
    src_sharpSAT/MainSolver/FormulaCache.cpp
)

target_link_libraries(dsharp_exec
    ${GMPXX_LIBRARIES}
    ${GMP_LIBRARIES}
)

