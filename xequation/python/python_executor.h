#pragma once

#include <string>

#include "python_common.h"
#include "core/equation_common.h"

namespace xequation
{
namespace python
{
class PythonExecutor {
 public:
  PythonExecutor();
  ~PythonExecutor();
  
  // Disallow copy and assign.
  PythonExecutor(const PythonExecutor&) = delete;
  PythonExecutor& operator=(const PythonExecutor&) = delete;
  
  // Executes Python code string in the given local dictionary.
  InterpretResult Exec(const std::string& code_string, const pybind11::dict& local_dict = pybind11::dict());
  
  // Evaluates Python expression in the given local dictionary.
  InterpretResult Eval(const std::string& expression, const pybind11::dict& local_dict = pybind11::dict());

  void Interrupt()
  {
     pybind11::gil_scoped_acquire acquire;
     // get current thread state
      PyThreadState* tstate = current_thread_state_.load(std::memory_order_acquire);
      if (tstate)
      {
          PyThreadState_SetAsyncExc(tstate->thread_id, PyExc_KeyboardInterrupt);
      }
  }

  std::atomic<PyThreadState*>
      current_thread_state_{nullptr};  // Saved Python thread state for this executor.
};
} // namespace python
} // namespace xequation