//
// Created by CanhDo on 2024/12/09.
//

#ifndef STATEGENERATOR_HPP
#define STATEGENERATOR_HPP

#include "dd/Package.hpp"

class StateGenerator {
public:
    explicit StateGenerator(const std::size_t s);

    ~StateGenerator() = default;

    template<class DDPackage = dd::Package<> >
    pc::VectorDD
    generateRandomState(std::unique_ptr<DDPackage> &dd, const std::size_t totalQubits);

private:
    std::size_t seed = 0U;
    std::mt19937_64 mt;
};

#endif //STATEGENERATOR_HPP
