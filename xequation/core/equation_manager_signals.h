#pragma once

#include "bitmask.hpp"

namespace xequation
{
enum class EquationField
{
    kContent = 0x01,
    kType = 0x02,
    kStatus = 0x04,
    kMessage = 0x08,
    kDependencies = 0x10,
};

BITMASK_DEFINE_MAX_ELEMENT(EquationField, kDependencies)

enum class EquationGroupField
{
    kStatement = 0x01,
    kEquations = 0x02,
};

BITMASK_DEFINE_MAX_ELEMENT(EquationGroupField, kEquations)

class EquationManagerSignals
{
public:
};

} // namespace xequation
