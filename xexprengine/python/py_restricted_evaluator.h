#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "py_common.h"

// use RestrictedPython to ensure safety
namespace xexprengine
{
class PyRestrictedEvaluator
{
  public:
    PyRestrictedEvaluator();
    ~PyRestrictedEvaluator() = default;
    py::object Eval(const std::string &code, py::dict local);

    bool RegisterBuiltin(const std::string builtin);

    const std::unordered_set<std::string> &global_symbols()
    {
        return global_symbols_;
    }

  private:
    void SetupRestrictedEnvironment();

  private:
    py::dict safe_globals_;
    std::unordered_set<std::string> global_symbols_;

    py::module restrictedpython_module_;
    py::module restrictedpython_eval_module_;
    py::module restrictedpython_guards_module_;
    py::module restrictedpython_utilities_module_;
    py::module builtins_module_;
};
} // namespace xexprengine