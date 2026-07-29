//
// Created by CanhDo on 2024/12/17.
//

#ifndef STATETRANSITIONGRAPH_HPP
#define STATETRANSITIONGRAPH_HPP

#include "SearchGraph.hpp"
#include "Search.hpp"
#include "ast/WhileStmNode.hpp"
#include "dd/SimulationBase.hpp"
#include "utility/Timer.hpp"
#include "utility/macros.hpp"

class StateTransitionGraph : public SearchGraph, public Search {
public:
    StateTransitionGraph(SyntaxProg *currentProg, SimulationBase *ddSim, ExpNode *propExp, Search::Type type, int numSols,
                         int maxDepth, bool probMode = false);

    void handleInCache(int currStateId, int nextStateId) override;

    void search() override;

    bool checkSearchCondition() override;

    void checkState(State *s, const Timer &timer) override;

    void printExploredStates(const Timer &timer) const override;

    void printCommand() override;

    void dump() const override;

protected:
    void printSearchTiming(State *s, const Timer &timer) const;

    void printState(State *s, bool recursive = true) const;

    // for iterative methods
    void gaussSeidelMethod(int maxIter = 100000, qc::fp tol = 1e-9);

    void jacobiMethod(int maxIter = 100000, qc::fp tol = 1e-9);

    // solves the reachability linear system directly over the field Q[w]
    void exactGaussianElimination();

    std::unordered_set<int> backwardReachable();

private:
    bool probMod;
    ExpNode *propExp{nullptr};
    Type searchType{Type::ARROW_EXCLAMATION};
    int numSols{UNBOUNDED};
    int solutionCount{0};
    int depthBound{UNBOUNDED};

    // for probabilistic calculation
    std::unordered_map<int, qc::fp> probTab{}; // maps stateNr to the probability of reaching a target state
    std::unordered_set<int> targetStates{}; // set of target states
};
#endif//STATETRANSITIONGRAPH_HPP
