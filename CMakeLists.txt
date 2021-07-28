cmake_minimum_required(VERSION 3.13)
project(MetaLibReducer)
set(CMAKE_CXX_STANDARD 17)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/libs)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

include_directories("${INCLUDE_DIR}")
link_directories("${LIB_DIR}")

find_package(Clang REQUIRED)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(mfReduce
    ${SRC_DIR}/reduceMetaTest.cpp
    ${SRC_DIR}/reductionEngine.cpp
    ${SRC_DIR}/opportunitiesGatherer.cpp
    ${SRC_DIR}/compileAndExecute.cpp
    ${SRC_DIR}/reductionStep.cpp
    ${SRC_DIR}/reductionPass.cpp
    ${SRC_DIR}/MRGatherer.cpp
    ${SRC_DIR}/helperFunctions.cpp
    ${SRC_DIR}/fuzzingReduction.cpp
    ${SRC_DIR}/globals.cpp
    ${SRC_DIR}/logging.cpp
    )

if (NOT LLVM_ENABLE_RTTI)
    message(STATUS "LLVM compiled with RTTI disabled...")
    target_compile_options(mfReduce PUBLIC -fno-rtti)
endif()

#target_compile_options(mtFuzzer PUBLIC -stdlib=libc++)
#target_link_options(mtFuzzer PUBLIC -stdlib=libc++ -lc++abi)
#if (METALIB_ASAN)
    #target_compile_options(mtFuzzer PUBLIC -fsanitize=address)
    #target_link_options(mtFuzzer PUBLIC -fsanitize=address -fno-omit-frame-pointer)
#endif()

target_link_libraries(mfReduce
    LLVM
    clangAST
    clangASTMatchers
    clangBasic
    clangDriver
    clangFrontend
    clangRewriteFrontend
    clangSerialization
    clangStaticAnalyzerFrontend
    clangTooling
    )
