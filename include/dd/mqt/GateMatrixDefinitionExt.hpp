//
// Created by CanhDo on 2025/06/10.
//

#ifndef GATEMATRIXDEFINITIONEXT_HPP
#define GATEMATRIXDEFINITIONEXT_HPP

#include "dd/DDDefinitions.hpp"

constexpr dd::TwoQubitGateMatrix PHI_PLUS_MAT{
    {
        {0.5, 0, 0, 0.5},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0.5, 0, 0, 0.5}
    }
};

constexpr dd::TwoQubitGateMatrix PHI_MINUS_MAT{
    {
        {0.5, 0, 0, -0.5},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {-0.5, 0, 0, 0.5}
    }
};

constexpr dd::TwoQubitGateMatrix PSI_PLUS_MAT{
    {
        {0, 0, 0, 0},
        {0, 0.5, 0.5, 0},
        {0, 0.5, 0.5, 0},
        {0, 0, 0, 0}
    }
};

constexpr dd::TwoQubitGateMatrix PSI_MINUS_MAT{
    {
        {0, 0, 0, 0},
        {0, 0.5, -0.5, 0},
        {0, -0.5, 0.5, 0},
        {0, 0, 0, 0}
    }
};

#endif //GATEMATRIXDEFINITIONEXT_HPP
