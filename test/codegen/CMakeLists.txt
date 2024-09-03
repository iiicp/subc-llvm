enable_testing()

add_executable(
  codegen_test
  codegen_test.cc

  ../../lexer.cc 
  ../../type.cc 
  ../../diag_engine.cc
  ../../parser.cc 
  ../../sema.cc 
  ../../scope.cc
  ../../codegen.cc
  ../../eval_constant.cc
)

llvm_map_components_to_libnames(llvm_all Support Core ExecutionEngine MC MCJIT OrcJit native)

#message(STATUS "iiicp: ${llvm_all}")

target_link_libraries(
  codegen_test
  GTest::gtest_main
  ${llvm_all}
)

include(GoogleTest)
gtest_discover_tests(codegen_test)