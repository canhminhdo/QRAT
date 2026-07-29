//
// Created by Canh Minh Do on 2025/05/10.
//

#include "model/DTMC.hpp"
#include "Configuration.hpp"
#include "dd/exact/Dw.hpp"
#include <iomanip>
#include <sstream>

namespace {
// 30-significant-digit decimal of an exact probability (PRISM/Storm model
// syntax cannot express exact Q[w] values, so exactness necessarily ends here)
std::string exactProbToString(const Prob &p) {
    std::ostringstream os;
    os << std::setprecision(30)
       << (p.exact ? p.exact->toComplexFloat().real() : dd::exact::Float(p.raw()));
    return os.str();
}
}// namespace

DTMC::DTMC(SyntaxProg *currentProg, StateSpaceGraph *graphSearch) {
    this->currentProg = currentProg;
    this->graphSearch = graphSearch;
    std::string fileName = std::string(Token::name(currentProg->getName())) + ".pm";
    fileWriter = new FileWriter(fileName);
}

DTMC::~DTMC() {
    delete fileWriter;
}

void DTMC::addHeader() {
    if (Configuration::backend == Configuration::SimBackend::EXACT) {
        fileWriter->writeLine("// NOTE: transition probabilities are 30-digit decimal approximations of "
                              "exact Q[w] values, which are not expressible in PRISM syntax.");
    }
    fileWriter->writeLine("dtmc");
    fileWriter->writeEmptyLine();
    fileWriter->writeLine("module " + std::string(Token::name(currentProg->getName())));
}

void DTMC::buildModel() {
    Timer timer(true);
    addHeader();
    auto &seenStates = graphSearch->getSeenStates();
    fileWriter->writeLine("\ts : [0.." + std::to_string(seenStates.size() - 1) + "] init 0;");
    fileWriter->writeEmptyLine();
    int stateNr = 0;
    while (stateNr < seenStates.size()) {
        auto *state = seenStates[stateNr];
        auto &nextStates = state->nextStates;
        std::string transition = "\t[] s=" + std::to_string(stateNr) + " -> ";
        if (nextStates.size() > 0) {
            if (Configuration::backend != Configuration::SimBackend::EXACT) {
                for (auto it = nextStates.begin(); it != nextStates.end(); it++) {
                    const auto &[stateId, prob] = *it;
                    transition += std::to_string(prob.raw()) + ":(s'=" + std::to_string(stateId) + ")";
                    if (std::next(it) != nextStates.end()) {
                        transition += " + ";
                    }
                }
            } else {
                // print branches 1..n-1 as 30-digit decimals and the LAST
                // branch as the textual complement 1 - sum(printed values),
                // so the row sums to exactly 1 textually (the exact row
                // probabilities sum to exactly 1, but their printed
                // decimals are rounded)
                dd::exact::Float printedSum{0};
                for (std::size_t i = 0; i + 1 < nextStates.size(); i++) {
                    auto probStr = exactProbToString(nextStates[i].second);
                    printedSum += dd::exact::Float(probStr);
                    transition += probStr + ":(s'=" + std::to_string(nextStates[i].first) + ") + ";
                }
                auto last = dd::exact::Float(1) - printedSum;
                if (last < 0) {
                    last = 0;
                }
                std::ostringstream os;
                os << std::setprecision(45) << last;
                transition += os.str() + ":(s'=" + std::to_string(nextStates.back().first) + ")";
            }
            transition += ";";
            fileWriter->writeLine(transition);
        } else {
            // transition += "1.0:(s'=" + std::to_string(stateNr) + ")";
            // transition += ";";
            // fileWriter->writeLine(transition);
        }
        stateNr++;
    }
    fileWriter->writeEmptyLine();
    addFooter();
    this->printDtmcTiming(timer);
}

void DTMC::addFooter() {
    fileWriter->writeLine("endmodule");
    fileWriter->writeEmptyLine();
    addLabels();
    // fileWriter->writeEmptyLine();
    // addRewrads();
}

void DTMC::addLabels() {
    auto &prop2States = graphSearch->getProp2States();
    for (auto &[propId, stateIds]: prop2States) {
        std::string label = "label \"";
        label += Token::name(propId);
        label += "\" = ";
        for (int i = 0; i < stateIds.size(); i++) {
            label += "s=" + std::to_string(stateIds[i]);
            if (i != stateIds.size() - 1) {
                label += " | ";
            }
        }
        if (stateIds.size() == 0) {
            label += "false";
        }
        label += ";";
        fileWriter->writeLine(label);
    }
    auto &ending = graphSearch->getEndingStates();
    if (ending.size() > 0) {
        std::string endingLabel = "label \"ending\" = ";
        for (int i = 0; i < ending.size(); i++) {
            endingLabel += "s=" + std::to_string(ending[i]);
            if (i != ending.size() - 1) {
                endingLabel += " | ";
            }
        }
        endingLabel += ";";
        fileWriter->writeLine(endingLabel);
    }
}

void DTMC::addRewrads() {
    fileWriter->writeLine("rewards");
    fileWriter->writeLine("\t[] true: 1;");
    fileWriter->writeLine("endrewards");
}

FileWriter *DTMC::getFileModel() {
    return fileWriter;
}

void DTMC::printDtmcTiming(const Timer &timer) {
    std::cout << "\n";
    if (Configuration::showTiming) {
        Int64 real;
        Int64 virt;
        Int64 prof;
        if (timer.getTimes(real, virt, prof)) {
            std::cout << "Timer for DTMC model generation in " << prof / 1000 << "ms cpu (" << real / 1000 << "ms real)";
        }
    }
    std::cout << "\n";
}

void DTMC::cleanup() {
    if (fileWriter != nullptr) {
        std::string fileName = fileWriter->getFileName();
        std::remove(fileName.c_str());
    }
}
