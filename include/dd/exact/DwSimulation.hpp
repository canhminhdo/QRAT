//
// Created by CanhDo on 2026/07/20.
//

#ifndef DWSIMULATION_HPP
#define DWSIMULATION_HPP

#include "ast/PropExpNode.hpp"
#include "core/SyntaxProg.hpp"
#include "dd/SimulationBase.hpp"
#include "dd/exact/DwPackage.hpp"
#include <random>
#include <string>
#include <unordered_map>

// Exact simulation backend on top of exact-dd's DwPackage: amplitudes are
// exact elements of Q[w] (w = e^{i*pi/4}), so only Clifford+T gates are
// supported (validated up front). Post-measurement states are deliberately
// left unnormalized (1/sqrt(p) is not exact in D[w]); every measurement
// probability is computed as the exact ratio <Pv|Pv>/<v|v>.
class DwSimulation : public SimulationBase {
public:
    explicit DwSimulation(SyntaxProg *prog);

    ~DwSimulation() override = default;

    bool isExact() const override {
        return true;
    }

    QState getInitialState() const override;

    QState applyGate(UnitaryStmNode *stm, const QState &v) override;

    void incRef(const QState &v) override;

    void decRef(const QState &v) override;

    bool isUnreferenced(const QState &v) const override;

    bool garbageCollect(bool force = false) override;

    MeasureResult measureWithProb(MeasExpNode *expr, const QState &v) override;

    void printState(const QState &v) const override;

    std::string basisProb(const QState &v, const std::string &basis) const override;

    std::string basisAmplitude(const QState &v, const std::string &basis) const override;

    void dump() override;

protected:
    void ensureProjector(PropExpNode *propNode) override;

    bool testProp(const QState &v, PropExpNode *propNode) override;

private:
    using vEdge = dd::exact::DwVEdge;
    using mEdge = dd::exact::DwMEdge;
    using Dw = dd::exact::Dw;

    void validateProgram();

    void validateStmSeq(StmSeq *seq);

    // Throws with a clear message if the gate has no exact representation.
    void checkSupported(UnitaryStmNode *stm);

    const mEdge &buildGate(UnitaryStmNode *stm);

    mEdge buildProjector(PropExpNode *propNode);

    mEdge buildProjectorOne(PropExpNode *propNode);

    mEdge buildProjectorTwo(PropExpNode *propNode);

    void initQState();

    // basis[0] refers to the highest qubit (as in MQT's getValueByPath);
    // exact-dd's amplitude() takes bits[i] = qubit i.
    std::vector<bool> basisToBits(const std::string &basis) const;

    // exact dd package
    std::unique_ptr<dd::exact::DwPackage> dd;

    // storing initial values
    std::map<int, vEdge> initStateMap;
    vEdge initialState{};

    // for properties (cached projectors are incRef'd so GC keeps them)
    std::unordered_map<PropExpNode *, mEdge, PropHash, PropEqual> projectorMap;

    // gate DDs cached per statement (incRef'd so GC keeps them)
    std::unordered_map<UnitaryStmNode *, mEdge> gateCache;
};
#endif//DWSIMULATION_HPP
