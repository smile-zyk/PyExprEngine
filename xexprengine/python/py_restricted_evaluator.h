#pragma once

#include <string>
#include <unordered_set>

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
    bool AddModule(const std::string &module_name);
    bool AddCustomModule(const std::string &module_file_path);
    bool RemoveModule(const std::string &module_name);
    const std::unordered_set<std::string> &builtins()
    {
        return builtins_cache_;
    }
    const std::unordered_set<std::string> &active_modules()
    {
        return active_modules_;
    }

  private:
    void SetupRestrictedEnvironment();
    void CacheBuiltins();

  private:
    py::dict safe_globals_;
    std::unordered_set<std::string> active_modules_;
    std::unordered_set<std::string> builtins_cache_;

    py::module restrictedpython_module_;
    py::module restrictedpython_eval_module_;
    py::module restrictedpython_guards_module_;
    py::module restrictedpython_utilities_module_;
    py::module builtins_module_;
};
} // namespace xexprengine