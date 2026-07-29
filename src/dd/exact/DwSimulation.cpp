//
// Created by CanhDo on 2026/07/20.
//

#include "dd/exact/DwSimulation.hpp"
#include "Configuration.hpp"
#include "ast/AtomicStmNode.hpp"
#include "ast/CondStmNode.hpp"
#include "ast/InitExpNode.hpp"
#include "ast/KetExpNode.hpp"
#include "ast/WhileStmNode.hpp"
#include "core/Token.hpp"
#include "core/VarSymbol.hpp"
#include "dd/exact/DwGateMatrixDefinitions.hpp"
#include <set>
#include <sstream>

namespace {
const std::set<std::string> SINGLE_TARGET_GATES{
    "i", "x", "y", "z", "h", "s", "sdg", "t", "tdg", "v", "vdg", "sx", "sxdg"};
const std::set<std::string> TWO_TARGET_GATES{"swap", "iswap", "iswapdg", "dcx"};

// exact 1/2 = 1 / sqrt(2)^2
const dd::exact::Dw HALF{1, 0, 0, 0, 2};
}// namespace

DwSimulation::DwSimulation(SyntaxProg *prog)
    : SimulationBase{prog}, dd{std::make_unique<dd::exact::DwPackage>(prog->getNqubits())} {
    validateProgram();
    initQState();
}

void DwSimulation::validateProgram() {
    validateStmSeq(prog->getStmSeq());
}

void DwSimulation::validateStmSeq(StmSeq *seq) {
    if (seq == nullptr || seq->getHead() == nullptr) {
        return;
    }
    auto *tail = seq->getTail();
    for (auto *stm = seq->getHead(); stm != nullptr; stm = (stm == tail) ? nullptr : stm->getNext()) {
        if (auto *unitaryStm = dynamic_cast<UnitaryStmNode *>(stm)) {
            checkSupported(unitaryStm);
        } else if (auto *condStm = dynamic_cast<CondStmNode *>(stm)) {
            validateStmSeq(condStm->getThenStm());
            validateStmSeq(condStm->getElseStm());
        } else if (auto *whileStm = dynamic_cast<WhileStmNode *>(stm)) {
            validateStmSeq(whileStm->getBody());
        } else if (auto *atomicStm = dynamic_cast<AtomicStmNode *>(stm)) {
            validateStmSeq(atomicStm->getBody());
        }
    }
}

void DwSimulation::checkSupported(UnitaryStmNode *stm) {
    auto name = qc::toString(stm->getOpType());
    auto nTargets = stm->getTargets().size();
    bool supported = stm->getParams().empty() &&
                     ((nTargets == 1 && SINGLE_TARGET_GATES.count(name) != 0) ||
                      (nTargets == 2 && TWO_TARGET_GATES.count(name) != 0));
    if (!supported) {
        throw std::runtime_error(
            "Error: gate '" + name +
            "' is not supported by the exact backend (Clifford+T only: "
            "i, x, y, z, h, s, sdg, t, tdg, v, vdg, sx, sxdg, "
            "swap, iswap, iswapdg, dcx, and their controlled variants). "
            "Switch back with 'set backend mqt .'");
    }
}

void DwSimulation::initQState() {
    std::vector<VarSymbol *> vars = prog->getVars();
    for (const auto &var: vars) {
        auto qubit = static_cast<std::size_t>(qVarMap[var->getName()]);
        if (Node *node = var->getValue(); node != nullptr) {
            if (auto *ketNode = dynamic_cast<KetExpNode *>(node); ketNode != nullptr) {
                switch (ketNode->getType()) {
                    case KetType::KET_ZERO:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{false}, qubit);
                        break;
                    case KetType::KET_ONE:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{true}, qubit);
                        break;
                    case KetType::KET_PLUS:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<dd::exact::BasisState>{dd::exact::BasisState::Plus}, qubit);
                        break;
                    case KetType::KET_MINUS:
                        initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<dd::exact::BasisState>{dd::exact::BasisState::Minus}, qubit);
                        break;
                    case KetType::KET_RANDOM:
                        initStateMap[var->getName()] = dd->makeRandomSingleQubitState(mt, 10, qubit);
                        break;
                    default:
                        throw std::runtime_error("Only support initialization with |0>, |1>, |+>, |->, or random state");
                }
            }
        } else {
            // not initialized, then set to |0> as default
            initStateMap[var->getName()] = dd->makeBasisState(1, std::vector<bool>{false}, qubit);
        }
    }
    assert(!vars.empty());
    // building initial state in ascending qubit order; every per-variable
    // state is already positioned at its final qubit index, so the factors
    // are combined without re-indexing (incIdx = false)
    initialState = initStateMap[revQVarMap[0]];
    for (int i = 1; i < vars.size(); i++) {
        initialState = dd->kronecker(initStateMap[revQVarMap[i]], initialState, i, false);
    }
    dd->incRef(initialState);
}

QState DwSimulation::getInitialState() const {
    return initialState;
}

const DwSimulation::mEdge &DwSimulation::buildGate(UnitaryStmNode *stm) {
    auto it = gateCache.find(stm);
    if (it != gateCache.end()) {
        return it->second;
    }
    checkSupported(stm);
    auto name = qc::toString(stm->getOpType());
    std::vector<std::size_t> controls;
    for (int i = 0; i < stm->getControls().size(); i++) {
        controls.push_back(static_cast<std::size_t>(qVarMap[stm->getControls().at(i)->getName()]));
    }
    mEdge gate;
    if (stm->getTargets().size() == 1) {
        auto target = static_cast<std::size_t>(getQubit(stm->getTargets().at(0)));
        auto matrix = dd::exact::gates::byName(name);
        gate = dd->makeControlledSingleQubitGateDD(controls, target, matrix);
    } else {
        auto target0 = static_cast<std::size_t>(getQubit(stm->getTargets().at(0)));
        auto target1 = static_cast<std::size_t>(getQubit(stm->getTargets().at(1)));
        auto matrix = dd::exact::gates::twoQubitByName(name);
        gate = dd->makeControlledTwoQubitGateDD(controls, target0, target1, matrix);
    }
    // keep cached gates alive across garbage collections
    dd->incRef(gate);
    return gateCache.emplace(stm, gate).first->second;
}

QState DwSimulation::applyGate(UnitaryStmNode *stm, const QState &v) {
    const auto &gate = buildGate(stm);
    return QState{dd->multiply(gate, v.exact())};
}

void DwSimulation::incRef(const QState &v) {
    dd->incRef(v.exact());
}

void DwSimulation::decRef(const QState &v) {
    dd->decRef(v.exact());
}

bool DwSimulation::isUnreferenced(const QState &v) const {
    const auto &e = v.exact();
    return e.p != nullptr && e.p->ref == 0;
}

bool DwSimulation::garbageCollect(bool force) {
    return dd->garbageCollect(force);
}

SimulationBase::MeasureResult DwSimulation::measureWithProb(MeasExpNode *expr, const QState &v) {
    auto var = expr->getVar();
    auto target = static_cast<std::size_t>(qVarMap[var->getName()]);
    auto r0 = dd->measureOneQubit(v.exact(), target, false);
    auto r1 = dd->measureOneQubit(v.exact(), target, true);
    // <v|v> = <P0 v|P0 v> + <P1 v|P1 v>; stored states are unnormalized, so
    // dividing by the input state's squared norm at EVERY measurement is
    // what makes each probability the correct conditional probability
    auto norm = r0.probability + r1.probability;
    if (norm.isZero()) {
        throw std::runtime_error("Cannot measure the zero state");
    }
    auto invNorm = norm.inverse();
    return {QState{r0.state}, Prob{r0.probability * invNorm},
            QState{r1.state}, Prob{r1.probability * invNorm}};
}

void DwSimulation::ensureProjector(PropExpNode *propNode) {
    if (projectorMap.find(propNode) == projectorMap.end()) {
        auto projector = buildProjector(propNode);
        // keep cached projectors alive across garbage collections
        dd->incRef(projector);
        projectorMap[propNode] = projector;
    }
}

DwSimulation::mEdge DwSimulation::buildProjector(PropExpNode *propNode) {
    auto size = propNode->getVars().size();
    if (size == 1) {
        return buildProjectorOne(propNode);
    }
    if (size == 2) {
        return buildProjectorTwo(propNode);
    }
    throw std::runtime_error("Only support property with one or two variables");
}

DwSimulation::mEdge DwSimulation::buildProjectorOne(PropExpNode *propNode) {
    auto target = static_cast<std::size_t>(qVarMap[propNode->getVars().at(0)->getName()]);
    if (auto *ketNode = dynamic_cast<KetExpNode *>(propNode->getExpr())) {
        if (ketNode->getType() == KetType::KET_ZERO) {
            return dd->makeSingleQubitGateDD(target, {Dw::one(), Dw::zero(), Dw::zero(), Dw::zero()});
        }
        if (ketNode->getType() == KetType::KET_ONE) {
            return dd->makeSingleQubitGateDD(target, {Dw::zero(), Dw::zero(), Dw::zero(), Dw::one()});
        }
        if (ketNode->getType() == KetType::KET_PLUS) {
            return dd->makeSingleQubitGateDD(target, {HALF, HALF, HALF, HALF});
        }
        if (ketNode->getType() == KetType::KET_MINUS) {
            return dd->makeSingleQubitGateDD(target, {HALF, -HALF, -HALF, HALF});
        }
        throw std::runtime_error("Only support initialization with |0>, |1>, |+>, |->, or initial state");
    }
    if (auto *initNode = dynamic_cast<InitExpNode *>(propNode->getExpr())) {
        // |v><v| of the (exactly normalized) initial single-qubit state of the referenced variable
        const auto &init = initStateMap[initNode->getVar()->getName()];
        auto initQubit = static_cast<std::size_t>(qVarMap[initNode->getVar()->getName()]);
        std::vector<bool> bits0(nqubits, false);
        std::vector<bool> bits1(nqubits, false);
        bits1[initQubit] = true;
        auto a = dd->amplitude(init, bits0);
        auto b = dd->amplitude(init, bits1);
        return dd->makeSingleQubitGateDD(target, {a * a.conjugate(), a * b.conjugate(),
                                                   b * a.conjugate(), b * b.conjugate()});
    }
    throw std::runtime_error("Only support projector from |0>, |1>, |+>, |->, or the initial state");
}

DwSimulation::mEdge DwSimulation::buildProjectorTwo(PropExpNode *propNode) {
    auto target1 = static_cast<std::size_t>(qVarMap[propNode->getVars().at(0)->getName()]);
    auto target2 = static_cast<std::size_t>(qVarMap[propNode->getVars().at(1)->getName()]);
    const auto O = Dw::zero();
    if (auto *ketNode = dynamic_cast<KetExpNode *>(propNode->getExpr())) {
        if (ketNode->getType() == KetType::KET_PHI_PLUS) {
            return dd->makeTwoQubitGateDD(target1, target2,
                                           {HALF, O, O, HALF,
                                            O, O, O, O,
                                            O, O, O, O,
                                            HALF, O, O, HALF});
        }
        if (ketNode->getType() == KetType::KET_PHI_MINUS) {
            return dd->makeTwoQubitGateDD(target1, target2,
                                           {HALF, O, O, -HALF,
                                            O, O, O, O,
                                            O, O, O, O,
                                            -HALF, O, O, HALF});
        }
        if (ketNode->getType() == KetType::KET_PSI_PLUS) {
            return dd->makeTwoQubitGateDD(target1, target2,
                                           {O, O, O, O,
                                            O, HALF, HALF, O,
                                            O, HALF, HALF, O,
                                            O, O, O, O});
        }
        if (ketNode->getType() == KetType::KET_PSI_MINUS) {
            return dd->makeTwoQubitGateDD(target1, target2,
                                           {O, O, O, O,
                                            O, HALF, -HALF, O,
                                            O, -HALF, HALF, O,
                                            O, O, O, O});
        }
    }
    throw std::runtime_error("Only support projector from |phi+>, |phi->, |psi+>, or |psi->");
}

bool DwSimulation::testProp(const QState &v, PropExpNode *propNode) {
    auto projector = projectorMap[propNode];
    auto v1 = dd->multiply(projector, v.exact());
    return v.exact().p == v1.p;
}

void DwSimulation::printState(const QState &v) const {
    dd->printVector(v.exact());
}

std::vector<bool> DwSimulation::basisToBits(const std::string &basis) const {
    if (basis.size() != nqubits) {
        throw std::runtime_error("Error: basis string must have exactly " + std::to_string(nqubits) + " bits");
    }
    std::vector<bool> bits(nqubits, false);
    for (std::size_t j = 0; j < nqubits; j++) {
        if (basis[j] != '0' && basis[j] != '1') {
            throw std::runtime_error("Error: basis string must consist of 0s and 1s");
        }
        // basis[0] is the highest qubit; amplitude() expects bits[i] = qubit i
        bits[nqubits - 1 - j] = basis[j] == '1';
    }
    return bits;
}

std::string DwSimulation::basisProb(const QState &v, const std::string &basis) const {
    std::vector<bool> bits;
    try {
        bits = basisToBits(basis);
    } catch (const std::exception &e) {
        return e.what();
    }
    auto amp = dd->amplitude(v.exact(), bits);
    auto norm = dd->innerProduct(v.exact(), v.exact());
    auto p = amp.normSquared() * norm.inverse();
    std::ostringstream os;
    os << p.toString() << " (approx. " << static_cast<double>(p.toComplexFloat().real()) << ")";
    return os.str();
}

std::string DwSimulation::basisAmplitude(const QState &v, const std::string &basis) const {
    std::vector<bool> bits;
    try {
        bits = basisToBits(basis);
    } catch (const std::exception &e) {
        return e.what();
    }
    auto amp = dd->amplitude(v.exact(), bits);
    auto norm = dd->innerProduct(v.exact(), v.exact());
    std::ostringstream os;
    os << amp.toString() << " (of unnormalized state; squared norm = " << norm.toString() << ")";
    return os.str();
}

void DwSimulation::dump() {
    for (auto &qVar: qVarMap) {
        std::cout << Token::name(qVar.first) << " -> " << qVar.second << std::endl;
    }
    for (auto &refQVar: revQVarMap) {
        std::cout << refQVar.first << " -> " << Token::name(refQVar.second) << std::endl;
    }
    for (auto &qVarVal: initStateMap) {
        std::cout << Token::name(qVarVal.first) << " -> " << std::endl;
        dd->printVector(qVarVal.second);
    }
    std::cout << "Initial state: " << std::endl;
    dd->printVector(initialState);
    std::cout << "Projectors " << std::endl;
    for (auto &projector: projectorMap) {
        std::cout << "Property: " << std::endl;
        projector.first->dump(true);
        dd->printMatrix(projector.second);
    }
}
