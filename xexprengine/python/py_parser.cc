#include "py_parser.h"
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <string>

namespace xexprengine
{
const std::string PyParser::kExceptionCode = R"(
class SyntaxErrorException(Exception):
    pass

class MultipleStatementsException(Exception):
    pass

class UnsupportedTypeException(Exception):
    pass

class ImportException(Exception):
    pass

class AssignmentException(Exception):
    pass

class AnalysisException(Exception):
    pass
)";

const std::string PyParser::kAnalysisCode = R"(
import ast

class _StatementAnalyzer:
    def __init__(self):
        self.valid_types = {
            'FunctionDef': 'func',
            'ClassDef': 'class', 
            'Import': 'import',
            'ImportFrom': 'import_from',
            'Assign': 'var'
        }
    
    def analyze(self, code):
        try:
            tree = ast.parse(code)
            
            if len(tree.body) != 1:
                raise MultipleStatementsException(f"Only one statement allowed, found {len(tree.body)}")
            
            statement = tree.body[0]
            statement_type = type(statement).__name__
            
            if statement_type not in self.valid_types:
                valid_type_names = list(self.valid_types.keys())
                raise UnsupportedTypeException(f"Unsupported statement type: {statement_type}. Supported: {', '.join(valid_type_names)}")
            
            analyzer_method = getattr(self, f'_analyze_{statement_type}', None)
            if analyzer_method:
                return analyzer_method(statement, code)
            else:
                raise AnalysisException(f"Analyzer not implemented: {statement_type}")
                
        except SyntaxError as e:
            raise SyntaxErrorException(f"Syntax error: {e}")
        except Exception as e:
            raise AnalysisException(f"Unexpected error: {e}")
    
    def _analyze_FunctionDef(self, node, code):
        return {
            'name': node.name,
            'dependencies': [],
            'type': 'func',
            'content': code.strip()
        }
    
    def _analyze_ClassDef(self, node, code):
        return {
            'name': node.name,
            'dependencies': [],
            'type': 'class',
            'content': code.strip()
        }
    
    def _analyze_Import(self, node, code):
        if len(node.names) != 1:
            raise ImportException("Import statement can only import one module")
        
        alias = node.names[0]
        name = alias.asname if alias.asname else alias.name
        
        return {
            'name': name,
            'dependencies': [],
            'type': 'import',
            'content': code.strip()
        }
    
    def _analyze_ImportFrom(self, node, code):
        if len(node.names) != 1:
            raise ImportException("From...import statement can only import one symbol")
        
        alias = node.names[0]
        name = alias.asname if alias.asname else alias.name
        
        return {
            'name': name,
            'dependencies': [],
            'type': 'import_from',
            'content': code.strip()
        }
    
    def _analyze_Assign(self, node, code):
        if len(node.targets) != 1:
            raise AssignmentException("Assignment statement can only have one target variable")
        
        target = node.targets[0]
        if not isinstance(target, ast.Name):
            raise AssignmentException("Assignment target must be a simple variable name")
        
        dependencies = self._extract_dependencies(node.value)
        
        return {
            'name': target.id,
            'dependencies': dependencies,
            'type': 'var',
            'content': code.strip()
        }
    
    def _extract_dependencies(self, node):
        dependencies = []
        
        class DependencyVisitor(ast.NodeVisitor):
            def __init__(self, deps_list):
                self.deps_list = deps_list
            
            def visit_Name(self, node):
                if isinstance(node.ctx, ast.Load):
                    self.deps_list.append(node.id)
                self.generic_visit(node)
        
        visitor = DependencyVisitor(dependencies)
        visitor.visit(node)
        return list(set(dependencies))
)";

PyParser::PyParser()
{
    try
    {
        py::dict analyze_dict;
        analyze_dict["__builtins__"] = py::module::import("builtins");
        py::exec(kExceptionCode, analyze_dict);
        py::exec(kAnalysisCode, analyze_dict);

        py::object AnalyzerClass = analyze_dict["_StatementAnalyzer"];
        analyzer_ = AnalyzerClass();
    }
    catch (const py::error_already_set &e)
    {
        throw ParseException(ParseErrorCode::kAnalysisError, e.what());
    }
}

VariableType StringToType(const std::string &type_str)
{
    static const std::unordered_map<std::string, VariableType> type_map = {
        {"func", VariableType::kFuncDecl},
        {"class", VariableType::kClassDecl},
        {"var", VariableType::kVarDecl},
        {"import", VariableType::kImport},
        {"import_from", VariableType::kImportFrom}
    };

    auto it = type_map.find(type_str);
    if (it != type_map.end())
    {
        return it->second;
    }
    throw ParseException(ParseErrorCode::kUnsupportedType, "Unknown type string: " + type_str);
}

ParseVariable PyParser::AnalyzeStatement(const std::string &code)
{
    try
    {
        auto analyze_func = analyzer_.attr("analyze");
        py::dict result = analyze_func(code);

        ParseVariable var;
        var.name = result["name"].cast<std::string>();
        var.content = result["content"].cast<std::string>();
        var.type = StringToType(result["type"].cast<std::string>());

        py::list deps = result["dependencies"];
        for (const auto &dep : deps)
        {
            var.dependencies.push_back(dep.cast<std::string>());
        }

        return var;
    }
    catch (const py::error_already_set &e)
    {
        std::string error_type = e.type().attr("__name__").cast<std::string>();
        std::string error_msg = e.what();

        if (error_type == "SyntaxErrorException")
        {
            throw ParseException(ParseErrorCode::kSyntaxError, error_msg);
        }
        else if (error_type == "MultipleStatementsException")
        {
            throw ParseException(ParseErrorCode::kMultipleStatements, error_msg);
        }
        else if (error_type == "UnsupportedTypeException")
        {
            throw ParseException(ParseErrorCode::kUnsupportedType, error_msg);
        }
        else if (error_type == "ImportException")
        {
            throw ParseException(ParseErrorCode::kImportError, error_msg);
        }
        else if (error_type == "AssignmentException")
        {
            throw ParseException(ParseErrorCode::kAssignmentError, error_msg);
        }
        else if (error_type == "AnalysisException")
        {
            throw ParseException(ParseErrorCode::kAnalysisError, error_msg);
        }
        else
        {
            throw ParseException(ParseErrorCode::kAnalysisError, "Unknown error: " + error_msg);
        }
    }
}

} // namespace xexprengine