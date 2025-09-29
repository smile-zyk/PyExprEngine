#include "py_symbol_extractor.h"
#include "core/expr_common.h"
#include <pybind11/gil.h>

using namespace xexprengine;

ParseResult PySymbolExtractor::Extract(const std::string &py_code)
{
    auto it = parse_result_cache_.find(py_code);
    if (it != parse_result_cache_.end())
    {
        return it->second;
    }

    ParseResult result = Parse(py_code);

    parse_result_cache_[py_code] = result;

    if (parse_result_cache_.size() > max_cache_size_)
    {
        CleanupCache();
    }

    return result;
}

void PySymbolExtractor::ClearCache()
{
    parse_result_cache_.clear();
}

void PySymbolExtractor::SetMaxCacheSize(size_t max_size)
{
    max_cache_size_ = max_size;
}

size_t PySymbolExtractor::GetCacheSize()
{
    return parse_result_cache_.size();
}

ParseResult PySymbolExtractor::Parse(const std::string &py_code)
{
    ParseResult result;
    result.status = VariableStatus::kParseSuccess;

    try
    {
        py::gil_scoped_acquire acquire;
        py::object ast_module = py::module::import("ast");
        py::object parse_func = ast_module.attr("parse");
        py::object tree = parse_func(py_code);

        VisitNode(tree, result);
    }
    catch (const py::error_already_set &e)
    {
        result.status = VariableStatus::kParseSyntaxError;
        result.parse_error_message = e.what();
    }

    return result;
}

void PySymbolExtractor::VisitName(py::handle node, ParseResult &result)
{
    py::object ctx_attr = node.attr("ctx");
    std::string ctx_type = py::cast<std::string>(ctx_attr.attr("__class__").attr("__name__"));
    std::string id_name = py::cast<std::string>(node.attr("id"));

    if (ctx_type == "Load" || ctx_type == "Store")
    {
        result.variables.insert(id_name);
    }
    GenericVisit(node, result);
}

void PySymbolExtractor::VisitCall(py::handle node, ParseResult &result)
{
    py::object func_attr = node.attr("func");
    std::string func_type = py::cast<std::string>(func_attr.attr("__class__").attr("__name__"));

    if (func_type == "Name")
    {
        std::string func_name = py::cast<std::string>(func_attr.attr("id"));
        result.functions.insert(func_name);
    }

    GenericVisit(node, result);
}

void PySymbolExtractor::GenericVisit(py::handle node, ParseResult &result)
{
    if (py::hasattr(node, "_fields"))
    {
        py::list fields = node.attr("_fields");
        for (auto field : fields)
        {
            std::string field_name = py::cast<std::string>(field);
            if (py::hasattr(node, field_name.c_str()))
            {
                py::object child = node.attr(field_name.c_str());
                VisitNode(child, result);
            }
        }
    }
}

void PySymbolExtractor::VisitNode(py::handle node, ParseResult &result)
{
    if (node.is_none())
        return;

    std::string node_type = py::cast<std::string>(node.attr("__class__").attr("__name__"));

    if (node_type == "Name")
    {
        VisitName(node, result);
    }
    else if (node_type == "Call")
    {
        VisitCall(node, result);
    }
    else
    {
        GenericVisit(node, result);
    }
}

void PySymbolExtractor::CleanupCache()
{
    size_t target_size = max_cache_size_ / 2;
    while (parse_result_cache_.size() > target_size)
    {
        parse_result_cache_.erase(parse_result_cache_.begin());
    }
}