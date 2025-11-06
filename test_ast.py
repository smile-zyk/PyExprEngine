import ast
import hashlib

def get_ast_signature(code_string):
    try:
        tree = ast.parse(code_string)
        
        def normalize_node(node):
            if isinstance(node, ast.AST):
                result = {}
                result['type'] = type(node).__name__
                for field, value in ast.iter_fields(node):
                    if field not in ['lineno', 'col_offset', 'end_lineno', 'end_col_offset']:
                        if isinstance(value, list):
                            result[field] = [normalize_node(item) for item in value]
                        elif isinstance(value, ast.AST):
                            result[field] = normalize_node(value)
                        else:
                            result[field] = value
                return result
            return node
        
        normalized_ast = normalize_node(tree)
        signature = str(normalized_ast)
        return hashlib.md5(signature.encode()).hexdigest()
        
    except SyntaxError:
        return hashlib.md5(code_string.encode()).hexdigest()

test_cases = [
    "x = 1 + 2",
    "x=1+2", 
    "x = 1+2",
    "def f(): return 42",
    "def f():return 42"
]

for code in test_cases:
    sig = get_ast_signature(code)
    print(f"{code:20} -> {sig}")