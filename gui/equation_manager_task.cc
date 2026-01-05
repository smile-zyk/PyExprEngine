#include "equation_manager_task.h"
#include "python/python_qt_wrapper.h"

namespace xequation 
{
namespace gui 
{

QVariant EquationManagerTask::Execute() 
{
    if(equation_manager_->language() == "Python") 
    {
        pybind11::gil_scoped_acquire acquire;
        PyThreadState* py_thread_state = PyThreadState_Get();
        internal_data_ = static_cast<void*>(py_thread_state);
    }

    return true;
}

void EquationManagerTask::RequestCancel() 
{
    Task::RequestCancel();
    if(equation_manager_->language() == "Python" && internal_data_) 
    {
        pybind11::gil_scoped_acquire acquire;
        void* data = internal_data_;
        PyThreadState* py_thread_state = static_cast<PyThreadState*>(data);
        PyThreadState_SetAsyncExc(py_thread_state->thread_id, PyExc_KeyboardInterrupt);
    }
}

void EquationManagerTask::Cleanup() 
{
    if(equation_manager_->language() == "python") 
    {
        internal_data_ = nullptr;
    }
    // TODO: Implement
}

UpdateEquationGroupTask::UpdateEquationGroupTask(EquationManager* manager, EquationGroupId group_id)
    : EquationManagerTask(manager), group_id_(group_id)
{
}

QVariant UpdateEquationGroupTask::Execute() 
{
    EquationManagerTask::Execute();
    SetProgress(0, "Starting update of equation group...");
    auto manager = equation_manager();
    // get the equations in the group before updating
    auto equation_names = manager->GetEquationGroup(group_id_)->GetEquationNames();
    auto update_equation_names = manager->graph().TopologicalSort(equation_names);

    SetProgress(10, "Updating equations in the group...");
    
    for(size_t i = 0; i < update_equation_names.size(); ++i) 
    {
        manager->UpdateEquationWithoutPropagate(update_equation_names[i]);
        if (IsCancelled()) 
        {
            SetProgress(100, "Update cancelled.");
            return false;
        }
        int progress = 10 + static_cast<int>(80.0 * (i + 1) / update_equation_names.size());
        SetProgress(progress, "Updated equation: " + QString::fromStdString(update_equation_names[i]));
    }

    SetProgress(100, "Update completed.");
    return true;
}
}
}
