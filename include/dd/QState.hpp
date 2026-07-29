//
// Created by CanhDo on 2026/07/20.
//

#ifndef QSTATE_HPP
#define QSTATE_HPP

#include "dd/Package_fwd.hpp"
#include "dd/exact/DwNode.hpp"
#include <utility>
#include <variant>

// Backend-neutral handle for a quantum state: either an MQT Core vector DD
// (floating-point backend) or an exact-dd vector DD (exact backend). The
// active alternative never changes within a run, since the backend is fixed
// at startup.
struct QState {
    std::variant<qc::VectorDD, dd::exact::DwVEdge> v;

    QState() = default;
    QState(qc::VectorDD e) : v{e} {}
    QState(dd::exact::DwVEdge e) : v{std::move(e)} {}

    // Canonical node pointer, used for state hashing/deduplication. Both
    // backends hash-cons nodes, so structurally identical states (up to a
    // global scalar factor) share one node pointer.
    [[nodiscard]] const void *node() const {
        if (const auto *e = std::get_if<qc::VectorDD>(&v)) {
            return e->p;
        }
        return std::get<dd::exact::DwVEdge>(v).p;
    }

    [[nodiscard]] bool isZeroTerminal() const {
        if (const auto *e = std::get_if<qc::VectorDD>(&v)) {
            return e->isZeroTerminal();
        }
        return std::get<dd::exact::DwVEdge>(v).isZeroTerminal();
    }

    [[nodiscard]] qc::VectorDD &mqt() { return std::get<qc::VectorDD>(v); }
    [[nodiscard]] const qc::VectorDD &mqt() const { return std::get<qc::VectorDD>(v); }
    [[nodiscard]] dd::exact::DwVEdge &exact() { return std::get<dd::exact::DwVEdge>(v); }
    [[nodiscard]] const dd::exact::DwVEdge &exact() const { return std::get<dd::exact::DwVEdge>(v); }
};

#endif//QSTATE_HPP
