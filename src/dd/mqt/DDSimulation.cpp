//
// Created by CanhDo on 2024/11/21.
//

#include "dd/mqt/DDSimulation.hpp"
#include "Configuration.hpp"
#include "ast/InitExpNode.hpp"
#include "ast/KetExpNode.hpp"
#include "ast/UnitaryStmNode.hpp"
#include "core/Token.hpp"
#include "core/VarSymbol.hpp"
#include "dd/Edge.hpp"
#include "dd/GateMatrixDefinitions.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "dd/mqt/GateMatrixDefinitionExt.hpp"
#include <sstream>

// using DDPackage = typename dd::Package<DDSimulationPackageConfig>;
DDSimulation::DDSimulation(SyntaxProg *prog) : SimulationBase{prog},
                                               dd{std::make_unique<DDPackage>(prog->getNqubits())} {
    initialize();
}

DDSimulation::~DDSimulation() {
    dd->garbageCollect(true);
}

QState DDSimulation::getInitialState() const {
    return initialState;
}

qc::VectorDD DDSimulation::generateRandomState() {
    // Uniform distribution for theta in [0, pi]
    std::uniform_real_distribution<dd::fp> dist_theta(0.0, dd::PI_4);
    // Uniform distribution for phi in [0, 2*pi]
    std::uniform_real_distribution<dd::fp> dist_phi(0.0, 2 * dd::PI_4);
    // Generate random theta and phi
    dd::fp theta = dist_theta(mt);
    dd::fp phi = dist_phi(mt);

    // Apply R_y and R_z gates to make a random state
    auto v = dd->makeBasisState(1, std::vector<bool>{false});
    auto gateRY = dd->makeGateDD(dd::ryMat(theta), 0);
    auto gateRZ = dd->makeGateDD(dd::rzMat(phi), 0);
    auto v1 = dd->multiply(gateRY, v);
    auto v2 = dd->multiply(gateRZ, v1);
    return v2;
}

void DDSimulation::ensureProjector(PropExpNode *propNode) {
    if (projectorMap.find(propNode) == projectorMap.end()) {
        projectorMap[propNode] = buildProjector(propNode);
        // projectorMap[propNode].printMatrix<dd::mNode>(nqubits);
    }
}

std::unordered_map<PropExpNode *, qc::MatrixDD, DDSimulation::PropHash, DDSimulation::PropEqual>
DDSimulation::getProjectorMap() {
    return projectorMap;
}

qc::Controls DDSimulation::buildControls(UnitaryStmNode *stm) {
    qc::Controls controls;
    for (int i = 0; i < stm->getControls().size(); i++) {
        auto cQubit = qVarMap[stm->getControls().at(i)->getName()];
        controls.insert({cQubit});
    }
    return controls;
}

qc::Targets DDSimulation::buildTargets(UnitaryStmNode *stm) {
    qc::Targets targets;
    for (int i = 0; i < stm->getTargets().size(); i++) {
        auto tQubit = getQubit(stm->getTargets().at(i));
        targets.push_back(tQubit);
    }
    return targets;
}

qc::MatrixDD DDSimulation::buildProjector(PropExpNode *propNode) {
    auto size = propNode->getVars().size();
    if (size == 1) {
        return buildProjectorOne(propNode);
    }
    if (size == 2) {
        return buildProjectorTwo(propNode);
    }
    throw std::runtime_error("Only support property with one or two variables");
}

qc::MatrixDD DDSimulation::buildProjectorOne(PropExpNode *propNode) {
    auto target = qVarMap[propNode->getVars().at(0)->getName()];
    if (auto *ketNode = dynamic_cast<KetExpNode *>(propNode->getExpr())) {
        if (ketNode->getType() == KetType::KET_ZERO) {
            auto v0 = dd->makeBasisState(1, std::vector<bool>{false});
            return dd->outerProduct(v0, target);
        }
        if (ketNode->getType() == KetType::KET_ONE) {
            auto v1 = dd->makeBasisState(1, std::vector<bool>{true});
            return dd->outerProduct(v1, target);
        }
        if (ketNode->getType() == KetType::KET_PLUS) {
            auto v1 = dd->makeBasisState(1, std::vector<dd::BasisStates>{dd::BasisStates::plus});
            return dd->outerProduct(v1, target);
        }
        if (ketNode->getType() == KetType::KET_MINUS) {
            auto v1 = dd->makeBasisState(1, std::vector<dd::BasisStates>{dd::BasisStates::minus});
            return dd->outerProduct(v1, target);
        }
        throw std::runtime_error("Only support initialization with |0>, |1>, |+>, |->, or initial state");
    }
    if (auto *initNode = dynamic_cast<InitExpNode *>(propNode->getExpr())) {
        return dd->outerProduct(initStateMap[initNode->getVar()->getName()], target);
    }
    throw std::runtime_error("Only support projector from |0>, |1>, |+>, |->, or the initial state");
}

qc::MatrixDD DDSimulation::buildProjectorTwo(PropExpNode *propNode) {
    auto target1 = qVarMap[propNode->getVars().at(0)->getName()];
    auto target2 = qVarMap[propNode->getVars().at(1)->getName()];
    // assert(target1 == target2 - 1);
    if (auto *ketNode = dynamic_cast<KetExpNode *>(propNode->getExpr())) {
        if (ketNode->getType() == KetType::KET_PHI_PLUS) {
            return dd->outerProduct(PHI_PLUS_MAT, target1, target2);
        }
        if (ketNode->getType() == KetType::KET_PHI_MINUS) {
            return dd->outerProduct(PHI_MINUS_MAT, target1, target2);
        }
        if (ketNode->getType() == KetType::KET_PSI_PLUS) {
            return dd->outerProduct(PSI_PLUS_MAT, target1, target2);
        }
        if (ketNode->getType() == KetType::KET_PSI_MINUS) {
            return dd->outerProduct(PSI_MINUS_MAT, target1, target2);
        }
    }
    throw std::runtime_error("Only support projector from |phi+>, |phi->, |psi+>, or |psi->");
}

void DDSimulation::initialize() {
    initQState();
}

void DDSimulation::initQState() {
    std::vector<VarSymbol *> vars = prog->getVars();
    for (const auto &var: vars) {
        if (Node *node = var->getValue(); node != nullptr) {
            if (auto *ketNode = dynamic_cast<KetExpNode *>(node); ketNode != nullptr) {
                switch (ketNode->getType()) {
                    case KetType::KET_ZERO:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{false});
                        break;
                    case KetType::KET_ONE:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{true});
                        break;
                    case KetType::KET_PLUS:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<dd::BasisStates>{dd::BasisStates::plus});
                        break;
                    case KetType::KET_MINUS:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<dd::BasisStates>{dd::BasisStates::minus});
                        break;
                    case KetType::KET_RANDOM:
                        initStateMap[var->getName()] = generateRandomState();
                        // if (initStateMap[var->getName()].p->ref == 0)
                        //     dd->incRef(initStateMap[var->getName()]);
                        break;
                    default:
                        throw std::runtime_error("Only support initialization with |0>, |1> or random state");
                }
            }
        } else {
            // not initialized, then set to |0> as default
            initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{false});
        }
    }
    assert(!vars.empty());
    // building initial state
    initialState = initStateMap[revQVarMap[0]];
    for (int i = 1; i < vars.size(); i++) {
        initialState = dd->kronecker(initStateMap[revQVarMap[i]], initialState, i);
    }
    dd->incRef(initialState);
}

qc::VectorDD DDSimulation::applyGate(UnitaryStmNode *stm, qc::VectorDD v) {
    auto controls = buildControls(stm);
    qc::Targets targets = buildTargets(stm);
    auto params = stm->getParams();
    qc::StandardOperation op = StandardOperation(controls, targets, stm->getOpType(), params);
    auto gate = dd::getDD<DDSimulationPackageConfig>(&op, *dd);
    auto v1 = dd->multiply(gate, v);
    return v1;
}

QState DDSimulation::applyGate(UnitaryStmNode *stm, const QState &v) {
    return applyGate(stm, v.mqt());
}

void DDSimulation::incRef(qc::VectorDD &v) {
    dd->incRef(v);
}

void DDSimulation::decRef(qc::VectorDD &v) {
    dd->decRef(v);
}

void DDSimulation::incRef(const QState &v) {
    dd->incRef(v.mqt());
}

void DDSimulation::decRef(const QState &v) {
    dd->decRef(v.mqt());
}

bool DDSimulation::isUnreferenced(const QState &v) const {
    return v.mqt().p->ref == 0;
}

bool DDSimulation::garbageCollect(bool force) {
    return dd->garbageCollect(force);
}

std::pair<qc::VectorDD, qc::VectorDD> DDSimulation::measure(MeasExpNode *expr, qc::VectorDD v) {
    auto var = expr->getVar();
    auto target = qVarMap[var->getName()];
    auto v0 = dd->measureOneQubit(v, target, true);
    auto v1 = dd->measureOneQubit(v, target, false);
    return {v0, v1};
}

SimulationBase::MeasureResult DDSimulation::measureWithProb(MeasExpNode *expr, const QState &v) {
    auto var = expr->getVar();
    auto target = qVarMap[var->getName()];
    auto e = v.mqt();
    auto [v0, pZero, v1, pOne] = dd->measureOneQubit(e, target);
    return {QState{v0}, Prob{pZero}, QState{v1}, Prob{pOne}};
}

qc::VectorDD DDSimulation::project(qc::MatrixDD projector, qc::VectorDD v) {
    return dd->multiply(projector, v);
}

bool DDSimulation::testProp(const QState &v, PropExpNode *propNode) {
    auto projector = projectorMap[propNode];
    auto v1 = dd->multiply(projector, v.mqt());
    if (v.mqt().p == v1.p) {
        return true;
    }
    return false;
    // todo: should check structure similarity during checking fidelity for fast comparison
    // auto fd = dd->fidelity(v1, v);
    // std::cout << "Fidelity: " << fd << std::endl;
    // return std::abs(fd - 1) < Configuration::fidelityThreshold;
}

qc::fp DDSimulation::fidelity(qc::VectorDD v1, qc::VectorDD v2) {
    return dd->fidelity(v1, v2);
}

void DDSimulation::printState(const QState &v) const {
    v.mqt().printVector<dd::vNode>();
}

std::string DDSimulation::basisProb(const QState &v, const std::string &basis) const {
    const auto c = v.mqt().getValueByPath(nqubits, basis);
    std::ostringstream os;
    os << std::norm(c);
    return os.str();
}

std::string DDSimulation::basisAmplitude(const QState &v, const std::string &basis) const {
    const auto c = v.mqt().getValueByPath(nqubits, basis);
    std::ostringstream os;
    os << c;
    return os.str();
}

void DDSimulation::dump() {
    for (auto &qVar: qVarMap) {
        std::cout << Token::name(qVar.first) << " -> " << qVar.second << std::endl;
    }
    for (auto &refQVar: revQVarMap) {
        std::cout << refQVar.first << " -> " << Token::name(refQVar.second) << std::endl;
    }
    for (auto &qVarVal: initStateMap) {
        std::cout << Token::name(qVarVal.first) << " -> " << std::endl;
        qVarVal.second.printVector<dd::vNode>();
    }
    std::cout << "Initial state: " << std::endl;
    initialState.printVector<dd::vNode>();
    std::cout << "Projectors " << std::endl;
    for (auto &projector: projectorMap) {
        std::cout << "Property: " << std::endl;
        projector.first->dump(true);
        projector.second.printMatrix<dd::mNode>(nqubits);
    }
}

void DDSimulation::analyze() {
    dd->analyze();
}
