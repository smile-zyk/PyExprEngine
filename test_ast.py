import ast

class SymbolExtractor(ast.NodeVisitor):
    def __init__(self):
        self.symbols = {
            'variables': set(),      # 普通变量符号
            'functions': set(),      # 函数调用符号（包含模块名）
            'modules': set()         # 导入的模块
        }
    
    def visit_Import(self, node):
        """处理 import 语句"""
        for alias in node.names:
            self.symbols['modules'].add(alias.name)
        self.generic_visit(node)
    
    def visit_ImportFrom(self, node):
        """处理 from ... import 语句"""
        if node.module:
            self.symbols['modules'].add(node.module)
        self.generic_visit(node)
    
    def visit_Name(self, node):
        """处理变量名"""
        if isinstance(node.ctx, ast.Load):
            # 如果是读取变量（使用变量）
            self.symbols['variables'].add(node.id)
        elif isinstance(node.ctx, ast.Store):
            # 如果是写入变量（赋值变量）
            self.symbols['variables'].add(node.id)
        self.generic_visit(node)
    
    def visit_Attribute(self, node):
        """处理属性访问（如 math.sin）"""
        # 递归获取完整的属性链（如 math.sin.cos 等）
        full_path = self._get_attribute_chain(node)
        self.generic_visit(node)
    
    def visit_Call(self, node):
        """处理函数调用"""
        if isinstance(node.func, ast.Attribute):
            # 处理模块函数调用（如 math.sin）
            full_path = self._get_attribute_chain(node.func)
            if full_path:
                self.symbols['functions'].add(full_path)
        elif isinstance(node.func, ast.Name):
            # 处理普通函数调用（如 sin）
            self.symbols['functions'].add(node.func.id)
        
        # 继续遍历参数中的符号
        self.generic_visit(node)
    
    def _get_attribute_chain(self, node):
        """递归获取属性访问的完整链"""
        if isinstance(node, ast.Attribute):
            # 递归获取左侧部分
            left_part = self._get_attribute_chain(node.value)
            if left_part:
                return f"{left_part}.{node.attr}"
            return node.attr
        elif isinstance(node, ast.Name):
            return node.id
        return None
    
    def get_symbols(self):
        """返回整理后的符号"""
        return {
            'variables': sorted(self.symbols['variables']),
            'functions': sorted(self.symbols['functions']),
            'modules': sorted(self.symbols['modules'])
        }

# 测试代码
test_code = """
a+b
"""

def extract_symbols_from_code(code_string):
    """从代码中提取符号"""
    try:
        tree = ast.parse(code_string)
        extractor = SymbolExtractor()
        extractor.visit(tree)
        return extractor.get_symbols()
    except SyntaxError as e:
        print(f"语法错误: {e}")
        return {'variables': [], 'functions': [], 'modules': []}

# 执行提取
symbols = extract_symbols_from_code(test_code)

print("提取到的符号:")
print("=" * 50)
print("变量:", symbols['variables'])
print("函数调用:", symbols['functions'])
print("导入的模块:", symbols['modules'])