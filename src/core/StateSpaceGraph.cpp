//
// Created by Canh Minh Do on 2025/06/08.
//

#include "core/StateSpaceGraph.hpp"

#include <Configuration.hpp>

StateSpaceGraph::StateSpaceGraph(SyntaxProg *currentProg, SimulationBase *ddSim, char *property, std::vector<char *> *args) {
    this->currentProg = currentProg;
    this->ddSim = ddSim;
    this->property = property;
    this->ddSim->initProperty2();
    for (auto &prop: currentProg->getPropTab().getPropTab()) {
        prop2States[prop.first] = std::vector<int>();
    }
    this->args = args;
}

StateSpaceGraph::~StateSpaceGraph() {
    delete[] property;
    for (auto arg: *args) {
        delete[] arg;
    }
    delete args;
}

void StateSpaceGraph::search() {
    Timer timer(true);
    buildInitialState();
    checkState(seenStates[0], timer);
    int savedStateId = 0;
    while (savedStateId < seenStates.size()) {
        State *currentState = seenStates[savedStateId];
        procState(currentState, timer);
        savedStateId++;
    }
    printExploredStates(timer);
    timer.stop();
}

bool StateSpaceGraph::checkSearchCondition() {
    return true;
}

void StateSpaceGraph::checkState(State *s, const Timer &timer) {
    for (auto &prop: currentProg->getPropTab().getPropTab()) {
        auto *propExp = prop.second;
        if (ddSim->test(s->current, propExp)) {
            prop2States[prop.first].push_back(s->stateNr);
        }
    }
}

void StateSpaceGraph::printExploredStates(const Timer &timer) const {
    std::cout << "\n";
    std::cout << "Time for state transition graph construction with states: " << seenStates.size();
    if (Configuration::showTiming) {
        Int64 real;
        Int64 virt;
        Int64 prof;
        if (timer.getTimes(real, virt, prof)) {
            std::cout << " in " << prof / 1000 << "ms cpu (" << real / 1000 << "ms real)";
        }
    }
    std::cout << std::endl;
}

void StateSpaceGraph::printCommand() {
    if (Configuration::systemMode == LOADING_FILE_MODE) {
        std::cout << "==========================================\n";
    }
    std::cout << "pcheck in ";
    std::cout << Token::name(currentProg->getName());
    std::cout << " with " << property << "";
    for (auto arg: *args) {
        std::cout << " " << arg;
    }
    std::cout << " ." << std::endl;
}

void StateSpaceGraph::dump() const {
    std::cout << "Initial state: \n";
    ddSim->printState(ddSim->getInitialState());
    std::cout << "State Transition Graph\n";
    std::cout << "-------------------\n";
    if (seenStates.size() != 0) {
        printState(seenStates.at(0));
    }
}

std::unordered_map<int, std::vector<int>> &StateSpaceGraph::getProp2States() {
    return prop2States;
}

std::vector<int> &StateSpaceGraph::getEndingStates() {
    return ending;
}

char *StateSpaceGraph::getProperty() {
    return property;
}