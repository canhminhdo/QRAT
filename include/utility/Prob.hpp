//
// Created by CanhDo on 2026/07/20.
//

#ifndef PROB_HPP
#define PROB_HPP

#include "dd/exact/Dw.hpp"
#include <memory>
#include <stdexcept>

// A transition probability. `value` is the plain double used directly by
// the MQT backend; the exact backend additionally carries the exact Dw
// value (an element of the field Q[w]) so that probabilistic
// reachability can be solved exactly.
struct Prob {
    std::shared_ptr<const dd::exact::Dw> exact{};

    Prob() = default;
    /*implicit*/ Prob(double v) : value{v} {}
    explicit Prob(dd::exact::Dw d) : exact{std::make_shared<const dd::exact::Dw>(std::move(d))} {}

    [[nodiscard]] bool isZero() const {
        return exact ? exact->isZero() : value == 0.0;
    }

    // Plain double for callers that only ever handle non-exact Probs.
    [[nodiscard]] double raw() const {
        if (exact) {
            throw std::logic_error("Prob::raw() called on an exact-backend probability; use approx() or exactValue()");
        }
        return value;
    }

    // Approximate probability as a double, always valid
    [[nodiscard]] double approx() const {
        return exact ? exact->toComplexDouble().real() : value;
    }

    // Exact value for the exact reachability solver.
    [[nodiscard]] dd::exact::Dw exactValue() const {
        if (exact) {
            return *exact;
        }
        if (value == 0.0) {
            return dd::exact::Dw::zero();
        }
        if (value == 1.0) {
            return dd::exact::Dw::one();
        }
        throw std::runtime_error("Prob: no exact value available for " + std::to_string(value));
    }

private:
    double value{0.0};
};

#endif//PROB_HPP
