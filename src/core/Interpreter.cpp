//
// Created by CanhDo on 2024/11/15.
//

#include "core/Interpreter.hpp"

#include <Configuration.hpp>

#include "dd/mqt/DDSimulation.hpp"
#include "dd/exact/DwSimulation.hpp"
#include "model/DTMC.hpp"
#include "model/PrismRunner.hpp"
#include "utility/Tty.hpp"
#include <core/StateSpaceGraph.hpp>
#include <iostream>
#include "model/RunnerFactory.hpp"
#include "dd/SimulationFactory.hpp"
#include <exception>

void Interpreter::setCurrentProg(Token progName) {
    currentProg = new SyntaxProg(progName);
}

SyntaxProg *Interpreter::getCurrentProg() const {
    return currentProg;
}

bool Interpreter::existProg(Token progName) {
    auto savedProg = savedProgs.find(progName.code());
    if (savedProg == savedProgs.end()) {
        return false;
    }
    currentProg = savedProg->second;
    return true;
}

void Interpreter::initDDSimulation() {
    if (currentProg) {
        try {
            ddSim = SimulationFactory::create(currentProg);
        } catch (const std::exception &e) {
            std::cout << Tty(Tty::RED) << e.what() << Tty(Tty::RESET) << std::endl;
            ddSim = nullptr;
        }
    } else {
        std::cerr << "Error: No program to simulate" << std::endl;
    }
}

void Interpreter::initGraphSearch(ExpNode *propExp, Search::Type type, int numSols, int maxDepth, bool probMode) {
    graphSearch = new StateTransitionGraph(currentProg, ddSim, propExp, type, numSols, maxDepth, probMode);
}

void Interpreter::initGraphSearch(char *property, std::vector<char *> *args) {
    graphSearch = new StateSpaceGraph(currentProg, ddSim, property, args);
}

void Interpreter::execute() {
    if (ddSim == nullptr || graphSearch == nullptr) {
        std::cout << Tty(Tty::RED) << "Error: no simulation available" << Tty(Tty::RESET) << std::endl;
        return;
    }
    graphSearch->printCommand();
    graphSearch->search();
}

void Interpreter::executePCheck() {
    if (ddSim == nullptr || graphSearch == nullptr || runner == nullptr) {
        std::cout << Tty(Tty::RED) << "Error: no simulation available" << Tty(Tty::RESET) << std::endl;
        if (runner != nullptr) {
            delete runner;
            runner = nullptr;
        }
        return;
    }
    Timer timer(true);
    auto *stateSpaceGraph = dynamic_cast<StateSpaceGraph *>(graphSearch);
    if (runner->isAvailable() && stateSpaceGraph != nullptr) {
        stateSpaceGraph->printCommand();
        stateSpaceGraph->search();
        auto dtmc = DTMC(currentProg, stateSpaceGraph);
        dtmc.buildModel();
        runner->modelCheck(dtmc.getFileModel()->getFileName(), std::string(stateSpaceGraph->getProperty()));
        if (!runner->getSaveModel()) {
            dtmc.cleanup();
        }
        if (Configuration::showTiming) timer.total();
    }
    delete runner;
    timer.stop();
}

void Interpreter::initializeSearch(int progName, ExpNode *propExp, Search::Type type, int numSols, int maxDepth, bool probMode) {
    assert(currentProg != nullptr && progName == currentProg->getName());
    cleanSearch();
    initDDSimulation();
    if (ddSim == nullptr) {
        return;
    }
    initGraphSearch(propExp, type, numSols, maxDepth, probMode);
}

void Interpreter::cleanSearch() {
    if (graphSearch != nullptr) {
        delete graphSearch;
        graphSearch = nullptr;
    }
    if (ddSim != nullptr) {
        delete ddSim;
        ddSim = nullptr;
    }
}

void Interpreter::initializePCheck(int progName, char *property, std::vector<char *> *args) {
    assert(currentProg != nullptr && progName == currentProg->getName());
    cleanSearch();
    initDDSimulation();
    if (ddSim == nullptr) {
        return;
    }
    initGraphSearch(property, args);
    initRunner(args);
}

void Interpreter::initRunner(std::vector<char *> *args) {
    runner = RunnerFactory::createRunner(args);
}

void Interpreter::finalizeProg() {
    assert(currentProg != nullptr);
    auto oldProg = savedProgs.find(currentProg->getName());
    if (oldProg == savedProgs.end()) {
        savedProgs.insert({currentProg->getName(), currentProg});
        std::cout << "==========================================\n";
        std::cout << "prog " << Token::name(currentProg->getName()) << std::endl;
    } else {
        delete oldProg->second;
        oldProg->second = currentProg;
        std::cout << "==========================================\n";
        std::cout << "prog " << Token::name(currentProg->getName()) << '\n';
        std::cout << Tty(Tty::GREEN) << "Advisory: " << Tty(Tty::RESET) << "redefining program " << Tty(Tty::MAGENTA) << Token::name(currentProg->getName()) << Tty(Tty::RESET) << "." << std::endl;
    }
}

void Interpreter::showPath(int stateId) {
    if (graphSearch != nullptr) {
        graphSearch->showPath(stateId);
    } else {
        std::cout << Tty(Tty::RED) << "Warning: " << Tty(Tty::RESET) << "no state graph." << std::endl;
    }
}

void Interpreter::showState(int stateId) {
    if (graphSearch != nullptr) {
        graphSearch->showState(stateId);
    } else {
        std::cout << Tty(Tty::RED) << "Warning: " << Tty(Tty::RESET) << "no state graph." << std::endl;
    }
}

void Interpreter::showBasisInfo(int stateId, std::string basis, bool isProb) {
    if (graphSearch != nullptr) {
        graphSearch->showBasisInfo(stateId, basis, isProb);
    } else {
        std::cout << Tty(Tty::RED) << "Warning: " << Tty(Tty::RESET) << "no state graph." << std::endl;
    }
}