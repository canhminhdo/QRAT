//
// Created by CanhDo on 2024/11/21.
//

#ifndef DDSIMULATION_HPP
#define DDSIMULATION_HPP

#include "DDSimulationPackageConfig.hpp"
#include "ast/MeasExpNode.hpp"
#include "ast/PropExpNode.hpp"
#include "ast/UnitaryStmNode.hpp"
#include "core/SyntaxProg.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Package_fwd.hpp"
#include "dd/SimulationBase.hpp"
#include "ir/QuantumComputation.hpp"
#include <unordered_map>

using qc::Control;
using qc::Controls;
using qc::OP_NAME_TO_TYPE;
using qc::Qubit;
using qc::StandardOperation;

class DDSimulation : public SimulationBase {
public:
    DDSimulation(SyntaxProg *prog);

    ~DDSimulation() override;

    bool isExact() const override {
        return false;
    }

    QState getInitialState() const override;

    qc::VectorDD generateRandomState();

    void initialize();

    void initQState();

    qc::Controls buildControls(UnitaryStmNode *stm);

    qc::Targets buildTargets(UnitaryStmNode *stm);

    qc::MatrixDD buildProjector(PropExpNode *propNode);

    qc::MatrixDD buildProjectorOne(PropExpNode *propNode);

    qc::MatrixDD buildProjectorTwo(PropExpNode *propNode);

    qc::VectorDD applyGate(UnitaryStmNode *stm, qc::VectorDD v);

    QState applyGate(UnitaryStmNode *stm, const QState &v) override;

    void incRef(qc::VectorDD &v);

    void decRef(qc::VectorDD &v);

    void incRef(const QState &v) override;

    void decRef(const QState &v) override;

    bool isUnreferenced(const QState &v) const override;

    bool garbageCollect(bool force = false) override;

    std::pair<qc::VectorDD, qc::VectorDD> measure(MeasExpNode *expr, qc::VectorDD v);

    MeasureResult measureWithProb(MeasExpNode *expr, const QState &v) override;

    qc::VectorDD project(qc::MatrixDD projector, qc::VectorDD v);

    qc::fp fidelity(qc::VectorDD v1, qc::VectorDD v2);

    void printState(const QState &v) const override;

    std::string basisProb(const QState &v, const std::string &basis) const override;

    std::string basisAmplitude(const QState &v, const std::string &basis) const override;

    void dump() override;

    void analyze();

    void checkQubitRange(dd::Qubit qubit) {
        if (qubit > nqubits) {
            throw std::runtime_error("Qubit index out of range");
        }
    }

    std::unordered_map<PropExpNode *, qc::MatrixDD, ::DDSimulation::PropHash, ::DDSimulation::PropEqual>
    getProjectorMap();

    ///---------------------------------------------------------------------------
    ///                            \n Operations \n
    ///---------------------------------------------------------------------------

#define DEFINE_SINGLE_TARGET_OPERATION(op)                                   \
    StandardOperation op(const Qubit target) {                               \
        return mc##op(Controls{}, target);                                   \
    }                                                                        \
    StandardOperation c##op(const Control &control, const Qubit target) {    \
        return mc##op(Controls{control}, target);                            \
    }                                                                        \
    StandardOperation mc##op(const Controls &controls, const Qubit target) { \
        checkQubitRange(target);                                             \
        return StandardOperation(controls, target, OP_NAME_TO_TYPE.at(#op)); \
    }

    DEFINE_SINGLE_TARGET_OPERATION(i)
    DEFINE_SINGLE_TARGET_OPERATION(x)
    DEFINE_SINGLE_TARGET_OPERATION(y)
    DEFINE_SINGLE_TARGET_OPERATION(z)
    DEFINE_SINGLE_TARGET_OPERATION(h)
    DEFINE_SINGLE_TARGET_OPERATION(s)
    DEFINE_SINGLE_TARGET_OPERATION(sdg)
    DEFINE_SINGLE_TARGET_OPERATION(t)
    DEFINE_SINGLE_TARGET_OPERATION(tdg)
    DEFINE_SINGLE_TARGET_OPERATION(v)
    DEFINE_SINGLE_TARGET_OPERATION(vdg)
    DEFINE_SINGLE_TARGET_OPERATION(sx)
    DEFINE_SINGLE_TARGET_OPERATION(sxdg)

#undef DEFINE_SINGLE_TARGET_OPERATION

protected:
    void ensureProjector(PropExpNode *propNode) override;

    bool testProp(const QState &v, PropExpNode *propNode) override;

private:
    // mqt dd package
    using DDPackage = typename dd::Package<DDSimulationPackageConfig>;
    std::unique_ptr<DDPackage> dd;

    // storing initial values
    using VectorDDMap = std::map<int, qc::VectorDD>;
    VectorDDMap initStateMap;
    qc::VectorDD initialState{};

    // for properties
    std::unordered_map<PropExpNode *, qc::MatrixDD, PropHash, PropEqual> projectorMap;
};
#endif//DDSIMULATION_HPP
