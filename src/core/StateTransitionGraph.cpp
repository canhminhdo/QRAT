//
// Created by CanhDo on 2024/12/18.
//

#include "core/StateTransitionGraph.hpp"
#include "ast/CondExpNode.hpp"
#include "core/global.hpp"
#include "utility/Tty.hpp"
#include "Configuration.hpp"
#include<queue>

StateTransitionGraph::StateTransitionGraph(SyntaxProg *currentProg, SimulationBase *ddSim, ExpNode *propExp,
                                           Search::Type type, int numSols, int maxDepth, bool probMode) {
    this->currentProg = currentProg;
    this->ddSim = ddSim;
    this->propExp = propExp;
    this->searchType = type;
    this->numSols = numSols;
    this->depthBound = maxDepth;
    this->probMod = probMode;
    this->ddSim->initProperty(propExp);
}

void StateTransitionGraph::handleInCache(int currStateId, int nextStateId) {
    if (probMod) {
        seenStates.at(nextStateId)->otherParents.push_back(currStateId);
    }
}

void StateTransitionGraph::search() {
    if (probMod) {
        DEBUG(
            std::cout << "Building state transition graph ...\n";
        )
    }
    Timer timer(true);
    buildInitialState();
    solutionCount = 0;
    int savedStateId = 0;
    if (isArrowStar(searchType)) {
        checkState(seenStates.at(0), timer);
    }
    while (solutionCount < numSols && savedStateId < seenStates.size()) {
        State *currentState = seenStates.at(savedStateId);
        if (currentState->depth >= depthBound || (isArrowOne(searchType) && currentState->depth >= 1)) {
            break;
        }
        procState(currentState, timer);
        savedStateId++;
    }
    if (probMod) {
        if (ddSim->isExact()) {
            exactGaussianElimination();
        } else {
            gaussSeidelMethod();
        }
    }
    printExploredStates(timer);
    timer.stop();
}

bool StateTransitionGraph::checkSearchCondition() {
    return solutionCount < numSols;
}

void StateTransitionGraph::checkState(State *s, const Timer &timer) {
    if (isArrowExclamation(searchType)) {
        if (!s->isFinalState())
            return;
    }
    if (ddSim->test(s->current, propExp)) {
        solutionCount++;
        if (probMod) {
            targetStates.insert(s->stateNr);
        } else {
            printSearchTiming(s, timer);
        }
    }
}

void StateTransitionGraph::printExploredStates(const Timer &timer) const {
    if (solutionCount != 0 && solutionCount >= numSols)
        return;
    std::cout << "\n";
    if (!probMod) {
        solutionCount == 0 ? std::cout << "No solution.\n" : std::cout << "No more solutions.\n";
    }
    std::cout << "states: " << seenStates.size();
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

void StateTransitionGraph::printCommand() {
    if (Configuration::systemMode == LOADING_FILE_MODE) {
        std::cout << "==========================================\n";
    }
    std::cout << (probMod ? "psearch in " : "search in ");
    std::cout << Token::name(currentProg->getName());
    std::cout << " with " << getSearchType(searchType) << " such that ";
    propExp->info();
    std::cout << " ." << std::endl;
}

void StateTransitionGraph::printSearchTiming(State *s, const Timer &timer) const {
    if (probMod)
        return;
    std::cout << "\n";
    std::cout << "Solution " << solutionCount << " (state " << s->stateNr << ")\n";
    std::cout << "states: " << seenStates.size();
    if (Configuration::showTiming) {
        Int64 real;
        Int64 virt;
        Int64 prof;
        if (timer.getTimes(real, virt, prof)) {
            std::cout << " in " << prof / 1000 << "ms cpu (" << real / 1000 << "ms real)";
        }
    }
    std::cout << "\n";
    std::cout << "quantum state: \n";
    ddSim->printState(s->current);
    std::cout.flush();
}

void StateTransitionGraph::dump() const {
    std::cout << "Initial state: \n";
    ddSim->printState(ddSim->getInitialState());
    std::cout << "Property: \n";
    propExp->dump();
    std::cout << "Search type: " << getSearchType(searchType) << "\n";
    std::cout << "Solution bound: ";
    if (numSols == UNBOUNDED) {
        std::cout << "unbounded\n";
    } else {
        std::cout << numSols << "\n";
    }
    std::cout << "Depth bound: ";
    if (depthBound == UNBOUNDED) {
        std::cout << "unbounded\n";
    } else {
        std::cout << depthBound << "\n";
    }
    // std::cout << "State Transition Graph\n";
    // std::cout << "-------------------\n";
    // if (seenStates.size() != 0) {
    //     printState(seenStates[0]);
    // }
}

void StateTransitionGraph::gaussSeidelMethod(int maxIter, qc::fp tol) {
    DEBUG(
        std::cout << "Running Gauss-Seidel method for probabilistic state transition graph...\n";
    );
    auto backwardStates = backwardReachable();
    for (int i = 0; i < maxIter; i++) {
        qc::fp maxDiff = 0.0;
        for (auto stateNr: backwardStates) {
            auto state = seenStates.at(stateNr);
            if (targetStates.find(state->stateNr) != targetStates.end()) {
                continue;
            }
            qc::fp newProb = 0.0;
            qc::fp oldProb = probTab[state->stateNr];
            for (const auto &[nextStateId, prob]: state->nextStates) {
                newProb += prob.raw() * probTab[nextStateId];
            }
            if (newProb > 1.0) {
                newProb = 1.0;
            }
            probTab[state->stateNr] = newProb;
            maxDiff = std::max(maxDiff, std::abs(newProb - oldProb));
        }
        if (maxDiff < tol) {
            DEBUG(
                std::cout << "Gauss-Seidel method converged after " << i + 1 << " iterations.\n";
            );
            break;
        }
    }
    std::cout << "\nResult: " << probTab[0] << std::endl;
}

void StateTransitionGraph::jacobiMethod(int maxIter, qc::fp tol) {
    DEBUG(
        std::cout << "Running Jacobi method for probabilistic state transition graph...\n";
    );
    auto backwardStates = backwardReachable();
    for (int i = 0; i < maxIter; i++) {
        qc::fp maxDiff = 0.0;
        std::unordered_map<int, qc::fp> newProbTab;
        for (auto stateNr: backwardStates) {
            auto state = seenStates.at(stateNr);
            if (targetStates.find(state->stateNr) != targetStates.end()) {
                newProbTab[state->stateNr] = 1.0;
                continue;
            }
            qc::fp newProb = 0.0;
            for (const auto &[nextStateId, prob]: state->nextStates) {
                newProb += prob.raw() * probTab[nextStateId];
            }
            if (newProb > 1.0) {
                newProb = 1.0;
            }
            newProbTab[state->stateNr] = newProb;
            maxDiff = std::max(maxDiff, std::abs(newProb - probTab[state->stateNr]));
        }
        probTab = std::move(newProbTab);
        if (maxDiff < tol) {
            DEBUG(
                std::cout << "Jacobi method converged after " << i + 1 << " iterations.\n";
            );
            break;
        }
    }
    std::cout << "\nResult: " << probTab[0] << std::endl;
}

void StateTransitionGraph::exactGaussianElimination() {
    DEBUG(
        std::cout << "Running exact Gaussian elimination for probabilistic state transition graph...\n";
    );
    using Dw = dd::exact::Dw;
    auto backwardStates = backwardReachable();
    Dw result = Dw::zero();
    if (targetStates.find(0) != targetStates.end()) {
        result = Dw::one();
    } else if (backwardStates.find(0) != backwardStates.end()) {
        // unknowns: states that can reach a target, excluding the targets
        // themselves; every state that cannot reach a target has exact
        // probability 0 and is dropped from the system, which makes
        // (I - A) nonsingular over the remaining states (its determinant is not zero)
        std::vector<int> unknowns;
        for (auto stateNr: backwardStates) {
            if (targetStates.find(stateNr) == targetStates.end()) {
                unknowns.push_back(stateNr);
            }
        }
        std::sort(unknowns.begin(), unknowns.end());
        std::unordered_map<int, std::size_t> pos;
        for (std::size_t i = 0; i < unknowns.size(); i++) {
            pos[unknowns[i]] = i;
        }
        const auto n = unknowns.size();
        // augmented system (I - A) x = b
        std::vector<std::vector<Dw>> mat(n, std::vector<Dw>(n + 1, Dw::zero()));
        for (std::size_t i = 0; i < n; i++) {
            mat[i][i] = Dw::one();
            auto *state = seenStates.at(unknowns[i]);
            for (const auto &[nextStateId, prob]: state->nextStates) {
                if (targetStates.find(nextStateId) != targetStates.end()) {
                    mat[i][n] += prob.exactValue();
                } else if (auto it = pos.find(nextStateId); it != pos.end()) {
                    mat[i][it->second] -= prob.exactValue();
                }
            }
        }
        // forward elimination with first-nonzero pivoting
        for (std::size_t k = 0; k < n; k++) {
            std::size_t pivot = k;
            while (pivot < n && mat[pivot][k].isZero()) {
                pivot++;
            }
            if (pivot == n) {
                throw std::runtime_error("Exact Gaussian elimination: singular system");
            }
            std::swap(mat[k], mat[pivot]);
            auto inv = mat[k][k].inverse();
            for (std::size_t j = k; j <= n; j++) {
                mat[k][j] *= inv;
            }
            for (std::size_t r = k + 1; r < n; r++) {
                if (mat[r][k].isZero()) {
                    continue;
                }
                auto factor = mat[r][k];
                for (std::size_t j = k; j <= n; j++) {
                    mat[r][j] -= factor * mat[k][j];
                }
            }
        }
        // back substitution (diagonal is 1 after scaling)
        std::vector<Dw> x(n, Dw::zero());
        for (std::size_t k = n; k-- > 0;) {
            auto val = mat[k][n];
            for (std::size_t j = k + 1; j < n; j++) {
                val -= mat[k][j] * x[j];
            }
            x[k] = val;
        }
        result = x[pos[0]];
    }
    std::cout << "\nResult: " << result.toString() << " (approx. " << result.toComplexDouble().real() << ")" << std::endl;
}

std::unordered_set<int> StateTransitionGraph::backwardReachable() {
    std::unordered_set<int> visited;
    std::queue<int> q;
    for (auto id: targetStates) {
        visited.insert(id);
        q.push(id);
        probTab[id] = 1.0;
    }
    int count = targetStates.size();
    while (!q.empty()) {
        int stateId = q.front();
        q.pop();
        if (count == 0) {
            probTab[stateId] = 0.0;
        } else {
            count--;
        }
        auto state = seenStates.at(stateId);
        auto processParent = [&](int parentId) {
            if (parentId != -1 && visited.insert(parentId).second) {
                q.push(parentId);
            }
        };
        processParent(state->parent);
        for (auto otherParentId : state->otherParents) {
            processParent(otherParentId);
        }
    }
    return visited;
}
