import ast
from typing import Dict, List, Set, Any, Optional

class StatementAnalyzer:
    def __init__(self):
        self.result = {
            'type': None,  # 'expression', 'function_def', 'import', 'from_import'
            'content': None,
            'generated_symbols': [],  # Symbols generated (function names, imported symbols, etc.)
            'dependencies': [],  # Variables/functions the expression depends on
            'error': None
        }
    
    def analyze_statement(self, code_statement: str) -> Dict[str, Any]:
        """Analyze a complete statement (can be multi-line) for type and content"""
        self.result = {
            'type': None,
            'content': code_statement.strip(),
            'generated_symbols': [],
            'dependencies': [],
            'error': None
        }
        
        if not code_statement.strip():
            self.result['error'] = 'Empty string'
            return self.result
        
        try:
            # Try to parse as a module
            tree = ast.parse(code_statement)
            
            # Check if it contains multiple statements
            if len(tree.body) > 1:
                self.result['error'] = f'Contains multiple statements ({len(tree.body)}), please provide a single complete statement'
                return self.result
            
            if len(tree.body) == 0:
                self.result['error'] = 'Empty statement'
                return self.result
            
            node = tree.body[0]
            
            if isinstance(node, ast.Expr):
                # Expression - does not generate new symbols
                self.result['type'] = 'expression'
                self._analyze_expression(node.value)
            elif isinstance(node, ast.FunctionDef):
                # Function definition - function name is generated symbol
                self.result['type'] = 'function_def'
                self.result['generated_symbols'] = [node.name]
                self._analyze_function_def(node)
            elif isinstance(node, ast.Import):
                # Import statement - imported module names (including aliases) are generated symbols
                self.result['type'] = 'import'
                self._analyze_import(node)
            elif isinstance(node, ast.ImportFrom):
                # From...import statement - imported symbols are generated symbols
                self.result['type'] = 'from_import'
                self._analyze_import_from(node)
            else:
                self.result['type'] = 'other'
                self.result['error'] = f'Unsupported analysis type: {type(node).__name__}'
                
        except SyntaxError as e:
            self.result['error'] = f'Syntax error: {e}'
        except Exception as e:
            self.result['error'] = f'Analysis error: {e}'
        
        return self.result
    
    def _analyze_expression(self, node: ast.AST) -> None:
        """Analyze expression, extract dependent variables and functions"""
        dependencies: Set[str] = set()
        
        class DependencyVisitor(ast.NodeVisitor):
            def __init__(self, dep_set: Set[str]):
                self.dependencies = dep_set
            
            def visit_Name(self, node: ast.Name) -> None:
                if isinstance(node.ctx, ast.Load):
                    self.dependencies.add(node.id)
                self.generic_visit(node)
            
            def visit_Attribute(self, node: ast.Attribute) -> None:
                # Handle attribute access, e.g., module.function
                if isinstance(node.value, ast.Name):
                    self.dependencies.add(node.value.id)
                self.generic_visit(node)
            
            def visit_Call(self, node: ast.Call) -> None:
                # Handle function calls
                if isinstance(node.func, ast.Name):
                    self.dependencies.add(node.func.id)
                elif isinstance(node.func, ast.Attribute):
                    if isinstance(node.func.value, ast.Name):
                        self.dependencies.add(node.func.value.id)
                self.generic_visit(node)
        
        visitor = DependencyVisitor(dependencies)
        visitor.visit(node)
        self.result['dependencies'] = sorted(list(dependencies))
    
    def _analyze_function_def(self, node: ast.FunctionDef) -> None:
        """Analyze function definition"""
        # Extract dependencies from function body
        dependencies: Set[str] = set()
        
        class FunctionDependencyVisitor(ast.NodeVisitor):
            def __init__(self, dep_set: Set[str]):
                self.dependencies = dep_set
            
            def visit_Name(self, node: ast.Name) -> None:
                if isinstance(node.ctx, ast.Load):
                    self.dependencies.add(node.id)
                self.generic_visit(node)
            
            def visit_Attribute(self, node: ast.Attribute) -> None:
                if isinstance(node.value, ast.Name):
                    self.dependencies.add(node.value.id)
                self.generic_visit(node)
        
        visitor = FunctionDependencyVisitor(dependencies)
        for stmt in node.body:
            visitor.visit(stmt)
        
        # Remove parameter names and function name itself
        args = [arg.arg for arg in node.args.args]
        if node.args.vararg:
            args.append(node.args.vararg.arg)
        if node.args.kwarg:
            args.append(node.args.kwarg.arg)
        
        dependencies.discard(node.name)
        for arg in args:
            dependencies.discard(arg)
        
        self.result['dependencies'] = sorted(list(dependencies))
    
    def _analyze_import(self, node: ast.Import) -> None:
        """Analyze import statement"""
        generated_symbols = []
        dependencies = set()  # Import statements have no dependencies
        
        for alias in node.names:
            # Use alias (if exists) or module name as generated symbol
            symbol = alias.asname if alias.asname else alias.name.split('.')[0]
            generated_symbols.append(symbol)
        
        self.result['generated_symbols'] = generated_symbols
        self.result['dependencies'] = []  # Import statements don't depend on other symbols
    
    def _analyze_import_from(self, node: ast.ImportFrom) -> None:
        """Analyze from...import statement"""
        generated_symbols = []
        dependencies = set()  # From import statements have no dependencies
        
        for alias in node.names:
            # Use alias (if exists) or symbol name as generated symbol
            symbol = alias.asname if alias.asname else alias.name
            generated_symbols.append(symbol)
        
        self.result['generated_symbols'] = generated_symbols
        self.result['dependencies'] = []  # From import statements don't depend on other symbols

def analyze_statement(code_statement: str) -> Dict[str, Any]:
    """
    Main function to analyze a complete statement
    
    Parameters:
        code_statement: Complete statement to analyze (can be multi-line)
        
    Returns:
        Dict containing analysis results
    """
    analyzer = StatementAnalyzer()
    return analyzer.analyze_statement(code_statement)

def print_analysis_result(result: Dict[str, Any]) -> None:
    """Print analysis results"""
    content_preview = result['content'][:100] + ('...' if len(result['content']) > 100 else '')
    print(f"Code: {content_preview}")
    print(f"Type: {result['type']}")
    
    if result['error']:
        print(f"Error: {result['error']}")
    else:
        if result['type'] == 'expression':
            print(f"Dependent variables/functions: {result['dependencies']}")
            print("Generated symbols: [] (expressions do not generate new symbols)")
        
        elif result['type'] == 'function_def':
            print(f"Function name: {result['generated_symbols'][0]}")
            print(f"Function dependencies (external variables/functions): {result['dependencies']}")
            print(f"Generated symbols: {result['generated_symbols']}")
        
        elif result['type'] == 'import':
            print(f"Imported symbols: {result['generated_symbols']}")
            print("Dependencies: [] (import statements do not depend on other symbols)")
        
        elif result['type'] == 'from_import':
            print(f"Imported symbols: {result['generated_symbols']}")
            print("Dependencies: [] (from import statements do not depend on other symbols)")
    
    print("-" * 50)

# Test examples
if __name__ == "__main__":
    test_cases = [
        # Expressions (do not generate symbols)
        "x + y * z",
        "math.sqrt(a) + len(b)",
        "func1().method2()",
        
        # Function definitions (generate function name symbol)
        "def simple_func(x): return x * 2",
        """def complex_function(x, y):
    external_var = 1
    result = x * y + external_var
    return result""",
        
        # Import statements (generate module symbols)
        "import numpy",
        "import pandas as pd",
        "import os, sys, math",
        "import matplotlib.pyplot as plt",
        
        # From import statements (generate imported symbols)
        "from math import sqrt, sin",
        "from collections import defaultdict as dd, Counter",
        "from sklearn.linear_model import LinearRegression as LR",
        
        # Multiple statements (should report error)
        "import os; import sys",
        "x = 1; y = 2",
        
        # Error cases
        "invalid code!!!",
        "",
    ]
    
    print("Statement analysis results (simplified version):\n")
    for i, test_case in enumerate(test_cases, 1):
        print(f"Test case {i}:")
        result = analyze_statement(test_case)
        print_analysis_result(result)