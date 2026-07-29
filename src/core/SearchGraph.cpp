//
// Created by Canh Minh Do on 2025/06/07.
//

#include "core/SearchGraph.hpp"
#include "ast/CondExpNode.hpp"
#include "ast/NumExpNode.hpp"
#include "core/global.hpp"
#include "utility/Tty.hpp"
#include <Configuration.hpp>
#include <ast/AtomicStmNode.hpp>
#include <iomanip>

SearchGraph::~SearchGraph() {
    for (auto state: seenStates) {
        delete state;
    }
    seenStates.clear();
    stateTab.clear();
}

void SearchGraph::buildInitialState() {
    auto *initialState = new State(currentProg->getStmSeq()->getHead(), ddSim->getInitialState());
    assert(seenStates.empty());
    makeState(initialState);
}

void SearchGraph::handleInCache(int currStateId, int nextStateId) {
}

void SearchGraph::procState(State *currentState, const Timer &timer) {
    StmNode *stm = currentState->pc;
    if (auto *endProg = dynamic_cast<EndStmNode *>(stm); endProg != nullptr && endProg->getNext() == nullptr) {
        return;
    }
    StmNode *nextStm = getNextStatement(stm);
    if (auto *skipStm = dynamic_cast<SkipStmNode *>(stm); skipStm != nullptr) {
        procSkipStm(skipStm, currentState, nextStm, timer);
    } else if (auto *unitaryStm = dynamic_cast<UnitaryStmNode *>(stm); unitaryStm != nullptr) {
        procUnitaryStm(unitaryStm, currentState, nextStm, timer);
    } else if (auto *condStm = dynamic_cast<CondStmNode *>(stm); condStm != nullptr) {
        procCondStm(condStm, currentState, timer);
    } else if (auto *whileStm = dynamic_cast<WhileStmNode *>(stm); whileStm != nullptr) {
        procWhileStm(whileStm, currentState, nextStm, timer);
    } else if (auto *atomicStm = dynamic_cast<AtomicStmNode *>(stm); atomicStm != nullptr) {
        procAtomicStm(atomicStm, currentState, nextStm, timer);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}
void SearchGraph::procSkipStm(SkipStmNode *skipStm, State *currentState, StmNode *nextStm, const Timer &timer) {
    auto [newState, inCache] = makeState(new State(nextStm, currentState->current, currentState->stateNr,
                                                   currentState->depth + 1));
    currentState->nextStates.push_back(std::make_pair(newState->stateNr, 1));
    if (!inCache) {
        checkState(newState, timer);
    } else {
        handleInCache(currentState->stateNr, newState->stateNr);
    }
}
void SearchGraph::procUnitaryStm(UnitaryStmNode *unitaryStm, State *currentState, StmNode *nextStm, const Timer &timer) {
    auto v = currentState->current;
    auto v1 = ddSim->applyGate(unitaryStm, v);
    if (ddSim->isUnreferenced(v1)) {
        ddSim->incRef(v1);
    }
    auto [newState, inCache] = makeState(new State(nextStm, v1, currentState->stateNr, currentState->depth + 1));
    currentState->nextStates.push_back(std::make_pair(newState->stateNr, 1));
    if (!inCache) {
        checkState(newState, timer);
    } else {
        handleInCache(currentState->stateNr, newState->stateNr);
    }
}
void SearchGraph::procCondStm(CondStmNode *condStm, State *currentState, const Timer &timer) {
    auto *condExp = dynamic_cast<CondExpNode *>(condStm->getCond());
    auto *measExp = dynamic_cast<MeasExpNode *>(condExp->getLeft());
    auto *numExp = dynamic_cast<NumExpNode *>(condExp->getRight());
    assert(condExp != nullptr && measExp != nullptr && numExp != nullptr);
    auto isZero = numExp->isZero();
    auto [v0, pZero, v1, pOne] = ddSim->measureWithProb(measExp, currentState->current);
    procCondBranch(currentState, isZero ? condStm->getThenStm()->getHead() : condStm->getElseStm()->getHead(), v0,
                   pZero, 0, timer);
    if (checkSearchCondition()) {
        procCondBranch(currentState, isZero ? condStm->getElseStm()->getHead() : condStm->getThenStm()->getHead(), v1,
                       pOne, 1, timer);
    }
}
void SearchGraph::procWhileStm(WhileStmNode *whileStm, State *currentState, StmNode *nextStm, const Timer &timer) {
    auto *condExp = dynamic_cast<CondExpNode *>(whileStm->getCond());
    auto *measExp = dynamic_cast<MeasExpNode *>(condExp->getLeft());
    auto *numExp = dynamic_cast<NumExpNode *>(condExp->getRight());
    assert(condExp != nullptr && measExp != nullptr && numExp != nullptr);
    auto isZero = numExp->isZero();
    auto [v0, pZero, v1, pOne] = ddSim->measureWithProb(measExp, currentState->current);
    procCondBranch(currentState, isZero ? whileStm->getBody()->getHead() : nextStm, v0, pZero, 0, timer);
    if (checkSearchCondition()) {
        procCondBranch(currentState, isZero ? nextStm : whileStm->getBody()->getHead(), v1, pOne, 1, timer);
    }
}
void SearchGraph::procAtomicStm(AtomicStmNode *atomicStm, State *currentState, StmNode *nextStm, const Timer &timer) {
    auto *body = atomicStm->getBody();
    auto *current = body->getHead();
    auto *tail = body->getTail();
    if (current == nullptr || tail == nullptr) {
        return;
    }
    auto v = currentState->current;
    ddSim->incRef(v);
    while (current != tail) {
        if (auto *skipStm = dynamic_cast<SkipStmNode *>(current); skipStm != nullptr) {
            // do nothing, skip statement
        } else if (auto *unitaryStm = dynamic_cast<UnitaryStmNode *>(current); unitaryStm != nullptr) {
            auto v1 = ddSim->applyGate(unitaryStm, v);
            ddSim->decRef(v);
            ddSim->incRef(v1);
            ddSim->garbageCollect();
            v = v1;
        } else {
            throw std::runtime_error("Unknown statement type in atomic statement");
        }
        current = current->getNext();
    }
    ddSim->garbageCollect(true);
    auto [newState, inCache] = makeState(new State(nextStm, v, currentState->stateNr, currentState->depth + 1));
    currentState->nextStates.push_back(std::make_pair(newState->stateNr, 1));
    if (!inCache) {
        checkState(newState, timer);
    } else {
        handleInCache(currentState->stateNr, newState->stateNr);
    }
}
void SearchGraph::procCondBranch(State *currentState, StmNode *nextStm, QState &v, const Prob &prob, int outcome, const Timer &timer) {
    if (prob.isZero() || v.isZeroTerminal()) {
        return;
    }
    if (ddSim->isUnreferenced(v)) {
        ddSim->incRef(v);
    }
    auto [newState, inCache] = makeState(new StateWithOutcome(nextStm, v, currentState->stateNr,
                                                              currentState->depth + 1, outcome));
    currentState->nextStates.push_back(std::make_pair(newState->stateNr, prob));
    if (!inCache) {
        checkState(newState, timer);
    } else {
        handleInCache(currentState->stateNr, newState->stateNr);
    }
}

StmNode *SearchGraph::getNextStatement(StmNode *stm) {
    StmNode *nextStm = stm->getNext();
    if (auto *endStm = dynamic_cast<EndStmNode *>(stm->getNext()); endStm != nullptr && endStm->getNext() != nullptr) {
        nextStm = endStm->getNext();
    }
    return nextStm;
}

void SearchGraph::showPath(int stateNr, bool endState) const {
    if (stateNr >= seenStates.size()) {
        std::cout << Tty(Tty::RED) << "Error: State ID is invalid" << Tty(Tty::RESET) << std::endl;
        return;
    }
    auto s = seenStates.at(stateNr);
    if (s->parent != -1) {
        showPath(s->parent, false);
    }
    if (s->hasOutcome()) {
        std::cout << "(with measurement result: " << s->getOutcome() << ")\n";
    }
    std::cout << "state " << s->stateNr << ", ";
    std::cout << "quantum state:\n";
    ddSim->printState(s->current);
    if (!endState && !s->isFinalState()) {
        std::cout << "===[ ";
        s->pc->info();
        std::cout << " ]===>\n";
    }
}

void SearchGraph::showState(int stateNr) const {
    if (stateNr >= seenStates.size()) {
        std::cout << Tty(Tty::RED) << "Error: State ID is invalid" << Tty(Tty::RESET) << std::endl;
        return;
    }
    auto s = seenStates.at(stateNr);
    if (s == nullptr) {
        return;
    }
    std::cout << "state " << s->stateNr << ", ";
    std::cout << "quantum state:\n";
    ddSim->printState(s->current);
}

void SearchGraph::showBasisInfo(int stateNr, std::string basis, bool isProb) const {
    if (stateNr >= seenStates.size()) {
        std::cout << Tty(Tty::RED) << "Error: State ID is invalid" << Tty(Tty::RESET) << std::endl;
        return;
    }
    auto s = seenStates.at(stateNr);
    if (s == nullptr) {
        return;
    }
    if (isProb) {
        std::cout << Tty(Tty::YELLOW) << "Probability of basis state |" << basis << "> in state " << stateNr << " is "<< ddSim->basisProb(s->current, basis) << Tty(Tty::RESET) << std::endl;
    } else {
        std::cout << Tty(Tty::YELLOW) << "Amplitude of basis state |" << basis << "> in state " << stateNr << " is "<< ddSim->basisAmplitude(s->current, basis) << Tty(Tty::RESET) << std::endl;
    }
}

void SearchGraph::printState(State *s, bool recursive) const {
    if (s == nullptr) {
        return;
    }
    std::cout << "-------------------\n";
    std::cout << "[State ID]: " << s->stateNr << "\n";
    std::cout << "[Parent State ID]: " << s->parent << "\n";
    std::cout << "[Program Counter]: ";
    s->pc->dump(false);
    std::cout << "[Depth]: " << s->depth << "\n";
    std::cout << "[Quantum State]: \n";
    ddSim->printState(s->current);
    std::cout << "[Next States]: ";
    for (auto [i, prob] : s->nextStates) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    if (recursive) {
        for (auto [i, prob]: s->nextStates) {
            if (i <= s->stateNr) {
                // do not print backward states
                std::cout << "Backward state detected\n";
                continue;
            };
            printState(seenStates.at(i));
        }
    }
}

std::pair<SearchGraph::State *, bool> SearchGraph::makeState(State *s) {
    auto it = stateTab.find(s);
    if (it != stateTab.end()) {
        // std::cout << "Found state in cache, then delete the newly created state\n";
        delete s;
        return {seenStates[it->second], true};
    }
    s->stateNr = seenStates.size();
    stateTab[s] = s->stateNr;
    seenStates.push_back(s);
    return {s, false};
}

std::vector<SearchGraph::State *> &SearchGraph::getSeenStates() {
    return seenStates;
}