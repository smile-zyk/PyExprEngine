#include "py_restricted_evaluator.h"
#include <pybind11/cast.h>
#include <pybind11/gil.h>
#include <pybind11/pytypes.h>

using namespace xexprengine;

PyRestrictedEvaluator::PyRestrictedEvaluator()
{
    py::gil_scoped_acquire acquire{};

    restrictedpython_module_ = py::module::import("RestrictedPython");
    restrictedpython_eval_module_ = py::module::import("RestrictedPython.Eval");
    restrictedpython_guards_module_ = py::module::import("RestrictedPython.Guards");
    restrictedpython_utilities_module_ = py::module::import("RestrictedPython.Utilities");
    builtins_module_ = py::module::import("builtins");

    SetupRestrictedEnvironment();
}

py::object PyRestrictedEvaluator::Eval(const std::string &code, py::dict local)
{
    try
    {
        py::gil_scoped_acquire acquire{};

        py::object compile_restricted = restrictedpython_module_.attr("compile_restricted");
        py::object bytecode = compile_restricted(code, "<string>", "eval");

        py::object eval_func = builtins_module_.attr("eval");
        py::object result = eval_func(bytecode, safe_globals_, local);

        return result;
    }
    catch (const py::error_already_set &e)
    {
        throw std::runtime_error("Python evaluation error: " + std::string(e.what()));
    }
}

// bool PyRestrictedEvaluator::ImportModule(const std::string &module_name)
// {
//     try
//     {
//         py::gil_scoped_acquire acquire{};
//         py::module module = py::module::import(module_name.c_str());

//         safe_globals_[module_name.c_str()] = module;

//         global_symbols_.insert(module_name);

//         return true;
//     }
//     catch (const py::error_already_set &e)
//     {
//         return false;
//     }
// }

// bool PyRestrictedEvaluator::ImportCustomModule(const std::string &module_file_path)
// {
//     try
//     {
//         py::gil_scoped_acquire acquire{};
//         py::module sys = py::module::import("sys");
//         sys.attr("path").attr("append")(py::str(module_file_path));

//         size_t last_slash = module_file_path.find_last_of("/\\");
//         size_t last_dot = module_file_path.find_last_of('.');

//         if (last_dot == std::string::npos)
//         {
//             return false;
//         }

//         std::string module_name = module_file_path.substr(
//             last_slash == std::string::npos ? 0 : last_slash + 1,
//             last_dot - (last_slash == std::string::npos ? 0 : last_slash + 1)
//         );

//         return ImportModule(module_name);
//     }
//     catch (const py::error_already_set &e)
//     {
//         return false;
//     }
// }

// bool PyRestrictedEvaluator::RemoveModule(const std::string &module_name)
// {
//     try
//     {
//         py::gil_scoped_acquire acquire{};
//         if (safe_globals_.contains(module_name.c_str()))
//         {
//             safe_globals_.attr("pop")(module_name.c_str());
//         }

//         auto it = global_symbols_.find(module_name);
//         if (it != global_symbols_.end())
//         {
//             global_symbols_.erase(it);
//             return true;
//         }

//         return false;
//     }
//     catch (const py::error_already_set &e)
//     {
//         return false;
//     }
// }

bool PyRestrictedEvaluator::RegisterBuiltin(const std::string builtin)
{
    py::object builtin_object = builtins_module_.attr(builtin.c_str());
    if(builtin_object.is_none() == true)
    {
        return false;
    }
    safe_globals_[builtin.c_str()] = builtin_object;
    global_symbols_.insert(builtin);
    return true;
}

void PyRestrictedEvaluator::SetupRestrictedEnvironment()
{
    py::gil_scoped_acquire acquire{};
    py::dict restricted_builtins = restrictedpython_module_.attr("safe_builtins").attr("copy")();

    safe_globals_ = py::dict();
    safe_globals_["__builtins__"] = restricted_builtins;

    safe_globals_["_getiter_"] = restrictedpython_eval_module_.attr("default_guarded_getiter");
    safe_globals_["_getitem_"] = restrictedpython_eval_module_.attr("default_guarded_getitem");
    safe_globals_["_unpack_sequence_"] = restrictedpython_guards_module_.attr("guarded_unpack_sequence");
    safe_globals_["_iter_unpack_sequence_"] = restrictedpython_guards_module_.attr("guarded_iter_unpack_sequence");
    safe_globals_["_write_"] = restrictedpython_guards_module_.attr("full_write_guard");
    safe_globals_["_getattr_"] = restrictedpython_eval_module_.attr("default_guarded_getattr");

    for (auto item : restricted_builtins)
    {
        std::string name = py::str(item.first).cast<std::string>();
        global_symbols_.insert(name);
    }
}