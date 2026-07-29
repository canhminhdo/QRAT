//
// Created by CanhDo on 2026/07/28.
//

#ifndef SIMULATIONFACTORY_HPP
#define SIMULATIONFACTORY_HPP

#include "dd/SimulationBase.hpp"
class SyntaxProg;

class SimulationFactory {
public:
    [[nodiscard]] static SimulationBase *create(SyntaxProg *prog);
};


#endif//SIMULATIONFACTORY_HPP
