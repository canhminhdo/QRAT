add_subdirectory(
    "${PROJECT_SOURCE_DIR}/extern/mqt-core"
    "extern/mqt-core"
    EXCLUDE_FROM_ALL)

add_subdirectory(
    "${PROJECT_SOURCE_DIR}/extern/exact-dd"
    "extern/exact-dd"
    EXCLUDE_FROM_ALL)

# COPY FILE
configure_file(prog.qw prog.qw)
configure_file(prog-loop.qw prog-loop.qw)
configure_file(lexerAux.cpp lexerAux.cpp)

# FLEX & BISON
find_package(BISON)
find_package(FLEX)
BISON_TARGET(Parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp COMPILE_FLAGS -Wcounterexamples) # Optional param: DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
FLEX_TARGET(Scanner lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.hpp)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

#message("CMAKE CURRENT SOURCE DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
#add_custom_command(
#    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexer.cpp
#    COMMAND flex++ -o ${CMAKE_CURRENT_SOURCE_DIR}/lexer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l)
#add_custom_target(lexer DEPENDS lexer.cpp)

# set include directory for parsing files
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#message(${BISON_Parser_OUTPUTS})
#message(${FLEX_Scanner_OUTPUTS})
#message(${FLEX_LIBRARIES})
#message(${FLEX_INCLUDE_DIRS})

add_library(
    Parser STATIC
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
    ../core/Token.cpp
    ../utility/StringTable.cpp
    ../core/Interpreter.cpp
    ../core/SyntaxProg.cpp
    ../core/global.cpp
    ../core/SymbolTable.cpp
    ../core/VarSymbol.cpp
    ../core/ConstSymbol.cpp
    ../ast/KetExpNode.cpp
    ../ast/QubitExpNode.cpp
    ../ast/OpExpNode.cpp
    ../ast/ConstExpNode.cpp
    ../ast/NumExpNode.cpp
    ../ast/UnitaryStmNode.cpp
    ../ast/StmSeq.cpp
    ../ast/SkipStmNode.cpp
    ../ast/MeasExpNode.cpp
    ../ast/CondExpNode.cpp
    ../ast/CondStmNode.cpp
    ../ast/WhileStmNode.cpp
    ../dd/SimulationBase.cpp
    ../dd/mqt/DDSimulation.cpp
    ../dd/exact/DwSimulation.cpp
    ../dd/SimulationFactory.cpp
    ../ast/CachedNode.cpp
    ../ast/EndStmNode.cpp
    ../core/SearchGraph.cpp
    ../core/StateSpaceGraph.cpp
    ../core/StateTransitionGraph.cpp
    ../ast/PropExpNode.cpp
    ../ast/InitExpNode.cpp
    ../ast/BoolExpNode.cpp
    ../utility/Timer.cpp
    ../utility/Tty.cpp
    ../utility/fileReader.cpp
    ../core/PropTable.cpp
    ../utility/FileWritter.cpp
    ../ast/AtomicStmNode.cpp
)

target_include_directories(Parser PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
#link the flex library if using default functions yywrap() and main()
#target_link_libraries(Parser PUBLIC ${FLEX_LIBRARIES})
target_link_libraries(Parser PUBLIC MQT::CoreDD ExactDD)
