#pragma once
#include "core/expr_engine.h"
#include "python/py_symbol_extractor.h"

namespace xexprengine
{
class PyExprEngine : public ExprEngine<PyExprEngine>
{
  public:
    struct PyEnvConfig
    {
        std::string py_home;
        std::vector<std::string> lib_path_list;
    };
    static void SetPyEnvConfig(const PyEnvConfig& config);
    EvalResult Evaluate(const std::string &expr, const ExprContext *context = nullptr) override;
    ParseResult Parse(const std::string &expr) override;
    std::unique_ptr<ExprContext> CreateContext() override;
  private:
    friend class ExprEngine<PyExprEngine>;

    void InitializePyEnv();
    void FinalizePyEnv();
    PyExprEngine();
    ~PyExprEngine() override;

  private:
    static PyEnvConfig config_;
    PySymbolExtractor symbol_extractor_;
    bool manage_python_context_ = false;
};
} // namespace xexprengine