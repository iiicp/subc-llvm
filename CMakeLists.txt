cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(Subc)
  set(SUBC_BUILT_STANDALONE TRUE)
endif()

if (SUBC_BUILT_STANDALONE) 
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_CXX_STANDARD 17)

    find_package(LLVM REQUIRED CONFIG)

    list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})

    include(AddLLVM)

    include_directories( . "${LLVM_BINARY_DIR}/include" "${LLVM_INCLUDE_DIR}")
    add_definitions(${LLVM_DEFINITIONS})
else()
    include_directories( . )
endif()

if (NOT ${LLVM_ENABLE_RTTI})
# For non-MSVC compilers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

set(LLVM_LINK_COMPONENTS ${LLVM_TARGETS_TO_BUILD} Support Core ExecutionEngine CodeGen MC MCJIT OrcJit native TargetParser)

add_llvm_executable(subc main.cc lexer.cc parser.cc print_visitor.cc  type.cc scope.cc sema.cc diag_engine.cc codegen.cc eval_constant.cc)

add_subdirectory(test)