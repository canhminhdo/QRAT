//
// Created by CanhDo on 2026/07/28.
//

#include "dd/SimulationFactory.hpp"

#include "Configuration.hpp"
#include "dd/exact/DwSimulation.hpp"
#include "dd/mqt/DDSimulation.hpp"

SimulationBase *SimulationFactory::create(SyntaxProg *prog) {
    switch (Configuration::backend) {
        case Configuration::SimBackend::EXACT:
            return new DwSimulation(prog);
        case Configuration::SimBackend::MQT:
        default:
            return new DDSimulation(prog);
    }
}