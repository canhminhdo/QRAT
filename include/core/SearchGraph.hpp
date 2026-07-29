//
// Created by Canh Minh Do on 2025/06/07.
//

#ifndef BASEGRAPH_HPP
#define BASEGRAPH_HPP

#include "ast/CondStmNode.hpp"
#include "ast/EndStmNode.hpp"
#include "ast/SkipStmNode.hpp"
#include "ast/StmNode.hpp"
#include "ast/WhileStmNode.hpp"
#include "dd/SimulationBase.hpp"
#include "utility/HashUtil.hpp"
#include "utility/Prob.hpp"
#include "utility/Timer.hpp"
#include <ast/AtomicStmNode.hpp>

class SearchGraph {
public:
    virtual ~SearchGraph();

    struct State {
        int stateNr;
        StmNode *pc;      // program counter
        QState current{};// current quantum state
        int parent;
        vector<int> otherParents; // other parents of this state;
        std::vector<std::pair<int, Prob>> nextStates;
        int depth{0};

        State(StmNode *pc, QState current, int parent = -1, int depth = 0)
            : pc{pc}, current{std::move(current)}, parent{parent}, depth{depth} {
        }

        bool isFinalState() {
            auto *endStm = dynamic_cast<EndStmNode *>(pc);
            if (endStm != nullptr && endStm->getNext() == nullptr) {
                return true;
            }
            return false;
        }

        virtual bool hasOutcome() const {
            return false;
        }

        virtual int getOutcome() const {
            throw std::runtime_error("No outcome available");
        }
    };

    struct StateWithOutcome : public State {
        int outcome{0};

        StateWithOutcome(StmNode *pc, QState current, int parent = -1, int depth = 0, int outcome = 0)
            : State(pc, std::move(current), parent, depth), outcome{outcome} {
        }

        bool hasOutcome() const override {
            return true;
        }
        int getOutcome() const override {
            return outcome;
        }
    };

    struct StateHash {
        std::size_t operator()(const State *s) const {
            return HashUtil::combinedHash(std::hash<StmNode *>()(s->pc),
                                          std::hash<const void *>()(s->current.node()));
        }
    };

    struct StateEqual {
        bool operator()(const State *lhs, const State *rhs) const {
            return lhs->pc == rhs->pc && lhs->current.node() == rhs->current.node();
        }
    };

    void buildInitialState();

    virtual void search() = 0;

    virtual void handleInCache(int currStateId, int nextStateId);

    virtual bool checkSearchCondition() = 0;

    void procState(State *currentState, const Timer &timer);

    void procSkipStm([[maybe_unused]] SkipStmNode *skipStm, State *currentState, StmNode *nextStm, const Timer &timer);

    void procUnitaryStm(UnitaryStmNode *unitaryStm, State *currentState, StmNode *nextStm, const Timer &timer);

    void procCondStm(CondStmNode *condStm, State *currentState, const Timer &timer);

    void procWhileStm(WhileStmNode *whileStm, State *currentState, StmNode *nextStm, const Timer &timer);

    void procAtomicStm(AtomicStmNode *atomicStm, State *currentState, StmNode *nextStm, const Timer &timer);

    void procCondBranch(State *currentState, StmNode *nextStm, QState &v, const Prob &prob, int outcome, const Timer &timer);

    StmNode *getNextStatement(StmNode *stm);

    void showPath(int stateNr, bool endState = true) const;

    void showState(int stateNr) const;

    void showBasisInfo(int stateNr, std::string prob, bool isProb = true) const;

    void printState(State *s, bool recursive = true) const;

    std::pair<State *, bool> makeState(State *s);

    virtual void checkState(State *s, const Timer &timer) = 0;

    virtual void printCommand() = 0;

    virtual void printExploredStates(const Timer &timer) const = 0;

    virtual void dump() const = 0;

    std::vector<State *> &getSeenStates();

protected:
    // for analysis
    std::unordered_map<State *, int, StateHash, StateEqual> stateTab;
    std::vector<State *> seenStates;

    SyntaxProg *currentProg;
    SimulationBase *ddSim;
};

#endif//BASEGRAPH_HPP
