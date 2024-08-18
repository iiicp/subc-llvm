#/bin/zsh

cd build
ninja
cd ..
./bin/lexer_test
./bin/parser_test
./bin/codegen_test