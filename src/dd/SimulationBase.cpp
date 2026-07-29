//
// Created by CanhDo on 2026/07/20.
//

#include "dd/SimulationBase.hpp"
#include "ast/BoolExpNode.hpp"
#include "ast/OpExpNode.hpp"
#include "core/Token.hpp"
#include "core/VarSymbol.hpp"
#include "Configuration.hpp"

SimulationBase::SimulationBase(SyntaxProg *prog) : prog{prog}, nqubits{prog->getNqubits()} {
    mt.seed(Configuration::seed);
    initQVarMap();
}

void SimulationBase::initQVarMap() {
    std::vector<VarSymbol *> vars = prog->getVars();
    // lexically sort variables by names in lexicographical order
    sort(vars.begin(), vars.end(), [](VarSymbol *v1, VarSymbol *v2) {
        std::string a = std::string(Token::name(v1->getName()));
        std::string b = std::string(Token::name(v2->getName()));
        // Find where the numeric part starts
        auto isDigit = [](char c) { return std::isdigit(c); };

        auto itA = std::find_if(a.begin(), a.end(), isDigit);
        auto itB = std::find_if(b.begin(), b.end(), isDigit);

        // Extract string prefix
        std::string prefixA(a.begin(), itA);
        std::string prefixB(b.begin(), itB);

        // Compare prefixes
        if (prefixA != prefixB) {
            return prefixA < prefixB;
        }

        // Extract numeric part
        int numA = std::stoi(std::string(itA, a.end()));
        int numB = std::stoi(std::string(itB, b.end()));

        // Compare numbers
        return numA < numB;
    });
    for (int i = 0; i < vars.size(); i++) {
        qVarMap.insert({vars.at(i)->getName(), i});
        revQVarMap.insert({i, vars.at(i)->getName()});
    }
}

qc::Qubit SimulationBase::getQubit(Symbol *symbol) {
    assert(dynamic_cast<VarSymbol *>(symbol) != nullptr);
    return qVarMap[symbol->getName()];
}

void SimulationBase::initProperty(ExpNode *expNode) {
    if (auto *propNode = dynamic_cast<PropExpNode *>(expNode)) {
        ensureProjector(propNode);
        return;
    }
    if (auto *opExpNode = dynamic_cast<OpExpNode *>(expNode)) {
        switch (opExpNode->getType()) {
            case OpExpType::NOT:
                initProperty(opExpNode->getRight());
                break;
            case OpExpType::AND:
            case OpExpType::OR:
                initProperty(opExpNode->getLeft());
                initProperty(opExpNode->getRight());
                break;
            default:
                throw std::runtime_error("Unsupported property type");
        }
    }
}

void SimulationBase::initProperty2() {
    for (auto &prop: prog->getPropTab().getPropTab()) {
        auto *expNode = prop.second;
        initProperty(expNode);
    }
}

bool SimulationBase::test(const QState &v, ExpNode *expNode) {
    if (auto *boolExp = dynamic_cast<BoolExpNode *>(expNode)) {
        if (boolExp->getVal() == BoolType::TRUE) {
            return true;
        } else if (boolExp->getVal() == BoolType::FALSE) {
            return false;
        } else {
            throw std::runtime_error("Unsupported boolean expression");
        }
    }
    if (auto *propNode = dynamic_cast<PropExpNode *>(expNode)) {
        return testProp(v, propNode);
    }
    if (auto *opExpNode = dynamic_cast<OpExpNode *>(expNode)) {
        switch (opExpNode->getType()) {
            case OpExpType::NOT:
                return not test(v, opExpNode->getRight());
            case OpExpType::AND:
                if (test(v, opExpNode->getLeft())) {
                    return test(v, opExpNode->getRight());
                }
                return false;
            case OpExpType::OR:
                if (test(v, opExpNode->getLeft())) {
                    return true;
                }
                return test(v, opExpNode->getRight());
            default:
                throw std::runtime_error("Unsupported property type");
        }
    }
    throw std::runtime_error("Unsupported expression type");
}
