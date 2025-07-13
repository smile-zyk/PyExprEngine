#include "PyExprEngine/pyexprengine.h"

int main()
{
    PyExprEngine::PyEnvConfig config;
    PyExprEngine::SetPyEnvConfig(config);
    PyExprEngine& engine = PyExprEngine::Instance();
    return 0;
}