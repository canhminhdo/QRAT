//
// Created by CanhDo on 2026/07/20.
//

#ifndef SIMULATIONBASE_HPP
#define SIMULATIONBASE_HPP

#include "ast/MeasExpNode.hpp"
#include "ast/PropExpNode.hpp"
#include "ast/UnitaryStmNode.hpp"
#include "core/SyntaxProg.hpp"
#include "dd/QState.hpp"
#include "utility/Prob.hpp"
#include <map>
#include <random>
#include <string>

// Abstract quantum-simulation backend. The graph search, interpreter, and
// model export only talk to this interface; DDSimulation (MQT Core,
// floating-point) and DwSimulation (exact-dd, exact arithmetic) implement it.
class SimulationBase {
public:
    explicit SimulationBase(SyntaxProg *prog);

    virtual ~SimulationBase() = default;

    [[nodiscard]] virtual bool isExact() const = 0;

    [[nodiscard]] virtual QState getInitialState() const = 0;

    [[nodiscard]] virtual QState applyGate(UnitaryStmNode *stm, const QState &v) = 0;

    virtual void incRef(const QState &v) = 0;

    virtual void decRef(const QState &v) = 0;

    [[nodiscard]] virtual bool isUnreferenced(const QState &v) const = 0;

    virtual bool garbageCollect(bool force = false) = 0;

    struct MeasureResult {
        QState zero;
        Prob pZero;
        QState one;
        Prob pOne;
    };

    [[nodiscard]] virtual MeasureResult measureWithProb(MeasExpNode *expr, const QState &v) = 0;

    void initProperty(ExpNode *expNode);

    void initProperty2();

    bool test(const QState &v, ExpNode *expNode);

    virtual void printState(const QState &v) const = 0;

    [[nodiscard]] virtual std::string basisProb(const QState &v, const std::string &basis) const = 0;

    [[nodiscard]] virtual std::string basisAmplitude(const QState &v, const std::string &basis) const = 0;

    virtual void dump() = 0;

    qc::Qubit getQubit(Symbol *symbol);

    struct PropHash {
        std::size_t operator()(const PropExpNode *node) const {
            return node->getHash();
        }
    };

    struct PropEqual {
        bool operator()(const PropExpNode *lhs, const PropExpNode *rhs) const {
            return lhs->isEqual(*rhs);
        }
    };

protected:
    // Build (and cache) the projector for a named property leaf.
    virtual void ensureProjector(PropExpNode *propNode) = 0;

    // Projector-invariance test P·v == v on a single property leaf.
    virtual bool testProp(const QState &v, PropExpNode *propNode) = 0;

    void initQVarMap();

    // program
    SyntaxProg *prog;
    std::size_t nqubits{};

    // mapping from variable to qubit and vice versa
    using QuantumVariableMap = std::map<int, int>;
    using RevQuantumVariableMap = std::map<int, int>;
    QuantumVariableMap qVarMap;
    RevQuantumVariableMap revQVarMap;

    // for random state generation
    std::mt19937 mt{};
};
#endif//SIMULATIONBASE_HPP
