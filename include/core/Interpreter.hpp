//
// Created by CanhDo on 2024/11/15.
//

#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "model/Runner.hpp"
#include "StateTransitionGraph.hpp"
#include "core/SyntaxProg.hpp"
#include "core/Token.hpp"
#include "dd/SimulationBase.hpp"
#include "core/SearchGraph.hpp"

class Interpreter {
public:
    Interpreter() = default;

    void setCurrentProg(Token progName);

    SyntaxProg *getCurrentProg() const;

    bool existProg(Token progName);

    void initDDSimulation();

    void initGraphSearch(ExpNode *propExp, Search::Type type, int numSols, int maxDepth, bool probMode = false);

    void initGraphSearch(char *property, std::vector<char *> *args);

    void execute();

    void executePCheck();

    void initializeSearch(int progName, ExpNode *propExp, Search::Type type, int numSols, int maxDepth,
                          bool probMode = false);

    void initializePCheck(int progName, char *property, std::vector<char *> *args);

    void initRunner(std::vector<char *> *args);

    void cleanSearch();

    void finalizeProg();

    void showPath(int stateId);

    void showState(int stateId);

    void showBasisInfo(int stateId, std::string basis, bool isProb = true);

private:
    SyntaxProg *currentProg{nullptr};
    SimulationBase *ddSim{nullptr};
    SearchGraph *graphSearch{nullptr};
    Runner *runner{nullptr};
    std::unordered_map<int, SyntaxProg *> savedProgs;
};
#endif//INTERPRETER_HPP
