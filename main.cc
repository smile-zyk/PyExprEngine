#include <pybind11/embed.h>
#include <iostream>
#include <Python.h>

namespace py = pybind11;

int main() {

    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    
    // 设置 Python 主目录
    PyConfig_SetString(&config, &config.home, L"D:/repos/PyExprEngine/build/vcpkg_installed/x64-windows/tools/python3");
    
    // 初始化解释器
    Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);    // 启动 Python 解释器并保持活动状态
    
    // 执行 Python 代码
    py::exec("print('Hello from pybind11 embedded Python!')");
    
    // 调用 Python 函数
    auto math = py::module_::import("math");
    double root = math.attr("sqrt")(25).cast<double>();
    std::cout << "Square root of 25 is: " << root << std::endl;
    
    // 从 Python 获取变量值
    py::exec("x = 42");
    auto x = py::globals()["x"].cast<int>();
    std::cout << "Python variable x = " << x << std::endl;
    
    // 执行 Python 脚本文件
    try {
        py::module_::import("script");  // 导入 script.py
    } catch (const py::error_already_set& e) {
        std::cerr << "Python error: " << e.what() << std::endl;
    }
    
    // 解释器会在 guard 析构时自动关闭
    return 0;
}