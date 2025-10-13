#include "py_expr_engine.h"
#include "py_restricted_evaluator.h"
#include "py_symbol_extractor.h"
#include "python/py_expr_context.h"
#include <memory>
#include <pybind11/eval.h>

using namespace xexprengine;

PyExprEngine::PyEnvConfig PyExprEngine::config_;

PyExprEngine::PyExprEngine()
{
    if (Py_IsInitialized())
    {
        manage_python_context_ = false;
    }
    else
    {
        manage_python_context_ = true;
        InitializePyEnv();
        const char *python_code = R"(
import sys
import os

def print_python_config():
    print("Python path configuration:")
    print(f"  PYTHONHOME = {os.environ.get('PYTHONHOME', '(not set)')}")
    print(f"  PYTHONPATH = {os.environ.get('PYTHONPATH', '(not set)')}")
    print(f"  program name = '{sys.argv[0] if len(sys.argv) > 0 else 'python'}'")
    print(f"  isolated = {1 if sys.flags.isolated else 0}")
    print(f"  environment = {0 if sys.flags.ignore_environment else 1}")
    print(f"  user site = {0 if sys.flags.no_user_site else 1}")
    print(f"  import site = {0 if sys.flags.no_site else 1}")
    print(f"  is in build tree = {0}")
    print(f"  stdlib dir = '{sys.prefix}/lib/python{sys.version_info.major}.{sys.version_info.minor}'")
    print(f"  sys._base_executable = '{getattr(sys, '_base_executable', sys.executable)}'")
    print(f"  sys.base_prefix = '{sys.base_prefix}'")
    print(f"  sys.base_exec_prefix = '{sys.base_exec_prefix}'")
    print(f"  sys.executable = '{sys.executable}'")
    print(f"  sys.prefix = '{sys.prefix}'")
    print(f"  sys.exec_prefix = '{sys.exec_prefix}'")
    
    print("  sys.path = [")
    for path in sys.path:
        print(f"    '{path}',")
    print("  ]")

if __name__ == "__main__":
    print_python_config()
)";

        py::exec(python_code);
        symbol_extractor_ = std::unique_ptr<PySymbolExtractor>(new PySymbolExtractor());
        restricted_evaluator_ = std::unique_ptr<PyRestrictedEvaluator>(new PyRestrictedEvaluator());
    }
}

void PyExprEngine::SetPyEnvConfig(const PyEnvConfig &config)
{
    config_ = config;
}

EvalResult PyExprEngine::Evaluate(const std::string &expr, const ExprContext *context)
{
    const PyExprContext *py_context = dynamic_cast<const PyExprContext *>(context);
    if (py_context == nullptr)
    {
        return EvalResult{std::string("Invalid context"), VariableStatus::kInvalidContext};
    }
    try
    {
        py::object res = restricted_evaluator_->Eval(expr, py_context->dict());
        EvalResult result;
        result.value = res;
        result.status = VariableStatus::kExprEvalSuccess;
        return result;
    }
    catch (const py::error_already_set &e)
    {
        return EvalResult{Value::Null(), VariableStatus::kExprEvalSyntaxError, e.what()};
    }
}

ParseResult PyExprEngine::Parse(const std::string &expr)
{
    return symbol_extractor_->Extract(expr, restricted_evaluator_->builtins(), restricted_evaluator_->active_modules());
}

std::unique_ptr<ExprContext> PyExprEngine::CreateContext()
{
    return std::unique_ptr<ExprContext>(new PyExprContext());
}

void PyExprEngine::InitializePyEnv()
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    if (!config_.py_home.empty())
    {
        config.home = Py_DecodeLocale(config_.py_home.c_str(), nullptr);
    }

    if (!config_.lib_path_list.empty())
    {
        config.module_search_paths_set = 1;
        for (const auto &path : config_.lib_path_list)
        {
            wchar_t *wide_path = Py_DecodeLocale(path.c_str(), nullptr);
            if (wide_path == nullptr)
            {
                PyErr_SetString(PyExc_RuntimeError, "Failed to decode Python path");
                throw std::runtime_error("Failed to decode Python path");
            }
            PyWideStringList_Append(&config.module_search_paths, wide_path);
            PyMem_RawFree(wide_path);
        }
    }

    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
    {
        if (status.err_msg)
        {
            fprintf(stderr, "Python initialization error: %s\n", status.err_msg);
        }
        throw std::runtime_error("Failed to initialize Python environment");
    }

    if (!Py_IsInitialized())
    {
        throw std::runtime_error("Python initialization failed");
    }
}

void PyExprEngine::FinalizePyEnv()
{
    Py_Finalize();
}

PyExprEngine::~PyExprEngine()
{
    if (manage_python_context_)
    {
        FinalizePyEnv();
    }
}