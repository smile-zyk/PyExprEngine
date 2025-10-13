#include "py_restricted_evaluator.h"
#include <pybind11/gil.h>
#include <pybind11/pytypes.h>

using namespace xexprengine;

PyRestrictedEvaluator::PyRestrictedEvaluator()
{
    try
    {
        py::gil_scoped_acquire acquire{};

        restrictedpython_module_ = py::module::import("RestrictedPython");
        restrictedpython_eval_module_ = py::module::import("RestrictedPython.Eval");
        restrictedpython_guards_module_ = py::module::import("RestrictedPython.Guards");
        restrictedpython_utilities_module_ = py::module::import("RestrictedPython.Utilities");
        builtins_module_ = py::module::import("builtins");

        SetupRestrictedEnvironment();
        CacheBuiltins();
    }
    catch (const py::error_already_set &e)
    {
        throw std::runtime_error("Python evaluation error: " + std::string(e.what()));
    }
}

py::object PyRestrictedEvaluator::Eval(const std::string &code, py::dict local)
{
    if (!Py_IsInitialized()) {
        throw std::runtime_error("Python interpreter not initialized");
    }
    
    if (_Py_IsFinalizing()) {
        throw std::runtime_error("Python interpreter is finalizing");
    }

    try
    {
        py::gil_scoped_acquire acquire{};

        py::object result = py::eval<pybind11::eval_single_statement>(code, safe_globals_, local);

        return result;
    }
    catch (const py::error_already_set &e)
    {
        throw std::runtime_error("Python evaluation error: " + std::string(e.what()));
    }
}

bool PyRestrictedEvaluator::AddModule(const std::string &module_name)
{
    try
    {
        py::gil_scoped_acquire acquire{};
        py::module module = py::module::import(module_name.c_str());

        safe_globals_[module_name.c_str()] = module;

        active_modules_.insert(module_name);

        return true;
    }
    catch (const py::error_already_set &e)
    {
        return false;
    }
}

bool PyRestrictedEvaluator::AddCustomModule(const std::string &module_file_path)
{
    try
    {
        py::gil_scoped_acquire acquire{};
        py::module sys = py::module::import("sys");
        sys.attr("path").attr("append")(py::str(module_file_path));

        size_t last_slash = module_file_path.find_last_of("/\\");
        size_t last_dot = module_file_path.find_last_of('.');

        if (last_dot == std::string::npos)
        {
            return false;
        }

        std::string module_name = module_file_path.substr(
            last_slash == std::string::npos ? 0 : last_slash + 1,
            last_dot - (last_slash == std::string::npos ? 0 : last_slash + 1)
        );

        return AddModule(module_name);
    }
    catch (const py::error_already_set &e)
    {
        return false;
    }
}

bool PyRestrictedEvaluator::RemoveModule(const std::string &module_name)
{
    try
    {
        py::gil_scoped_acquire acquire{};
        if (safe_globals_.contains(module_name.c_str()))
        {
            safe_globals_.attr("pop")(module_name.c_str());
        }

        auto it = active_modules_.find(module_name);
        if (it != active_modules_.end())
        {
            active_modules_.erase(it);
            return true;
        }

        return false;
    }
    catch (const py::error_already_set &e)
    {
        return false;
    }
}

void PyRestrictedEvaluator::SetupRestrictedEnvironment()
{
    py::gil_scoped_acquire acquire{};
    py::dict restricted_builtins = restrictedpython_module_.attr("safe_builtins").attr("copy")();

    restricted_builtins["sum"] = builtins_module_.attr("sum");
    restricted_builtins["abs"] = builtins_module_.attr("abs");
    restricted_builtins["round"] = builtins_module_.attr("round");
    restricted_builtins["min"] = builtins_module_.attr("min");
    restricted_builtins["max"] = builtins_module_.attr("max");
    restricted_builtins["float"] = builtins_module_.attr("float");
    restricted_builtins["int"] = builtins_module_.attr("int");
    restricted_builtins["len"] = builtins_module_.attr("len");
    restricted_builtins["str"] = builtins_module_.attr("str");

    safe_globals_ = py::dict();
    safe_globals_["__builtins__"] = restricted_builtins;

    safe_globals_["_getiter_"] = restrictedpython_eval_module_.attr("default_guarded_getiter");
    safe_globals_["_getitem_"] = restrictedpython_eval_module_.attr("default_guarded_getitem");
    safe_globals_["_unpack_sequence_"] = restrictedpython_guards_module_.attr("guarded_unpack_sequence");
    safe_globals_["_iter_unpack_sequence_"] = restrictedpython_guards_module_.attr("guarded_iter_unpack_sequence");
    safe_globals_["_write_guard_"] = restrictedpython_guards_module_.attr("full_write_guard");
}

void PyRestrictedEvaluator::CacheBuiltins()
{
    try
    {
        py::gil_scoped_acquire acquire{};
        py::dict builtins_dict = safe_globals_["__builtins__"].cast<py::dict>();

        for (auto item : builtins_dict)
        {
            std::string name = py::str(item.first).cast<std::string>();
            builtins_cache_.insert(name);
        }
    }
    catch (const py::error_already_set &e)
    {
        builtins_cache_.clear();
    }
}