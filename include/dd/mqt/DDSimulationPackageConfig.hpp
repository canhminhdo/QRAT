//
// Created by CanhDo on 2024/12/09.
//

#ifndef DDPACKAGECONFIG_HPP
#define DDPACKAGECONFIG_HPP

#include "dd/DDpackageConfig.hpp"

struct DDSimulationPackageConfig : public dd::DDPackageConfig {
    // simulation requires more resources for vectors.
    static constexpr std::size_t UT_VEC_NBUCKET            = 65'536U;
    static constexpr std::size_t CT_VEC_ADD_NBUCKET        = 65'536U;
    static constexpr std::size_t CT_MAT_VEC_MULT_NBUCKET   = 65'536U;
    static constexpr std::size_t CT_VEC_INNER_PROD_NBUCKET = 32'768U;

    // simulation only needs matrices for representing operations. Hence,
    // very little is needed here.
    static constexpr std::size_t UT_MAT_NBUCKET                 = 128U;
    static constexpr std::size_t UT_MAT_INITIAL_ALLOCATION_SIZE = 32U;

    // simulation needs no matrix addition, conjugate transposition,
    // matrix-matrix multiplication, or kronecker products.
    static constexpr std::size_t CT_MAT_ADD_NBUCKET        = 1U;
    static constexpr std::size_t CT_MAT_CONJ_TRANS_NBUCKET = 1U;
    static constexpr std::size_t CT_MAT_MAT_MULT_NBUCKET   = 1U;
    static constexpr std::size_t CT_VEC_KRON_NBUCKET       = 1U;
    static constexpr std::size_t CT_MAT_KRON_NBUCKET       = 1U;
};

#endif //DDPACKAGECONFIG_HPP
