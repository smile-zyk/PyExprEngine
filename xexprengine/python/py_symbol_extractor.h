#pragma once
#include <list>
#include <pybind11/pybind11.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "py_common.h"
#include "core/expr_common.h"

namespace xexprengine
{
class PySymbolExtractor
{
  public:
    PySymbolExtractor();
    ~PySymbolExtractor() = default;

    PySymbolExtractor(const PySymbolExtractor &) = delete;
    PySymbolExtractor(PySymbolExtractor &&) = delete;
    PySymbolExtractor &operator=(const PySymbolExtractor &) = delete;
    PySymbolExtractor &operator=(PySymbolExtractor &&) = delete;

    ParseResult Extract(const std::string& py_code, const std::unordered_set<std::string>& builtins = {}, const std::unordered_set<std::string>& modules = {});

    void ClearCache();

    void SetMaxCacheSize(size_t max_size);

    size_t GetCacheSize();

  private:
    ParseResult Parse(const std::string& py_code);

    void ProcessNameNode(py::handle node, ParseResult& result);
    void ProcessNode(py::handle node, ParseResult& result);

    void ProcessResultWithBuiltinsAndModules(ParseResult& result, const std::unordered_set<std::string>& builtins = {}, const std::unordered_set<std::string>& modules = {});

    void MoveToFront(const std::string& key);
    void EvictLRU();

  private:
    struct CacheEntry
    {
        std::string key;
        ParseResult value;
        CacheEntry(const std::string& k, const ParseResult& v) : key(k), value(v) {}
    };

    std::list<CacheEntry> cache_list_;
    std::unordered_map<std::string, typename std::list<CacheEntry>::iterator> cache_map_;
    size_t max_cache_size_ = 1000;
    py::module ast_module_;
};
} // namespace xexprengine