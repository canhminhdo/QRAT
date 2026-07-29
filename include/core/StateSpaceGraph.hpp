//
// Created by Canh Minh Do on 2025/06/08.
//

#ifndef STATESPACEGRAPH_HPP
#define STATESPACEGRAPH_HPP

#include "SearchGraph.hpp"

class StateSpaceGraph : public SearchGraph {
public:
    StateSpaceGraph(SyntaxProg *currentProg, SimulationBase *ddSim, char *property, std::vector<char *> *args);
    ~StateSpaceGraph() override;

    void search() override;
    bool checkSearchCondition() override;
    void checkState(State *s, const Timer &timer) override;
    void printExploredStates(const Timer &timer) const override;
    void printCommand() override;
    void dump() const override;
    std::unordered_map<int, std::vector<int>> &getProp2States();
    std::vector<int> &getEndingStates();
    char *getProperty();

private:
    char *property;
    std::unordered_map<int, std::vector<int>> prop2States;
    std::vector<int> ending;
    std::vector<char *> *args;
};

#endif//STATESPACEGRAPH_HPP
