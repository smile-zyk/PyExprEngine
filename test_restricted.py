from RestrictedPython import compile_restricted
from RestrictedPython.Guards import (
    safe_builtins,
    full_write_guard,
    guarded_setattr,
    safer_getattr
)

def create_secure_environment():
    """基于最新可用防护函数的安全环境配置"""
    
    # 自定义导入防护函数
    def custom_import_guard(name, *args, **kwargs):
        """替代 import_guard 的自定义实现"""
        allowed_modules = {
            'math', 'random', 'json', 'datetime', 
            'time', 'collections', 'itertools'
        }
        
        if name not in allowed_modules:
            raise ImportError(f"Import of '{name}' is not allowed")
        
        return __import__(name, *args, **kwargs)

    return {
        '__builtins__': safe_builtins,
        
        # 核心防护
        '_getattr_': guarded_setattr,
        '_write_': full_write_guard,
        '__import__': custom_import_guard,  # 使用自定义导入防护
        
        # 安全函数替代
        'getattr': safer_getattr,
        'hasattr': lambda obj, name: safer_getattr(obj, name, None) is not None,
        
        # 基础内置函数
        'isinstance': isinstance,
        'issubclass': issubclass,
        'len': len,
        'range': range,
        'list': list,
        'tuple': tuple,
        'dict': dict,
        'set': set,
        'str': str,
        'int': int,
        'float': float,
        'bool': bool,
        'sum': sum,
        'max': max,
        'min': min,
        'abs': abs,
        'round': round,
        
        # 预导入的安全模块
        'math': __import__('math'),
        'random': __import__('random'),
        'json': __import__('json'),
    }

def execute_safely(source_code, additional_globals=None):
    """安全执行代码"""
    
    restricted_globals = create_secure_environment()
    if additional_globals:
        restricted_globals.update(additional_globals)
    
    try:
        byte_code = compile_restricted(source_code, '<string>', 'exec')
        exec(byte_code, restricted_globals)
        return {'success': True, 'globals': restricted_globals}
    except Exception as e:
        return {'success': False, 'error': str(e)}

# 测试示例
if __name__ == "__main__":
    test_code = """
# 基本操作测试
numbers = [1, 2, 3, 4, 5]
squared = [x*x for x in numbers]
total = sum(squared)

# 类和属性测试
class Calculator:
    def __init__(self):
        self.factor = 2
    
    def calculate(self, x):
        return x * self.factor

calc = Calculator()
result = calc.calculate(5)

# 安全访问测试
value = getattr(calc, 'factor', None)

# 危险操作尝试
dangerous = []
try:
    import os
    dangerous.append('import succeeded')
except Exception as e:
    dangerous.append(f'import failed: {str(e)}')

try:
    eval('1+1')
    dangerous.append('eval succeeded')
except Exception as e:
    dangerous.append(f'eval failed: {str(e)}')

output = {
    'total': total,
    'result': result,
    'value': value,
    'dangerous_attempts': dangerous
}
"""

    result = execute_safely(test_code)
    if result['success']:
        print("执行成功！")
        print("计算结果:", result['globals']['output']['total'])
        print("类方法结果:", result['globals']['output']['result'])
        print("安全属性获取:", result['globals']['output']['value'])
        print("危险操作拦截:", result['globals']['output']['dangerous_attempts'])
    else:
        print("执行失败:", result['error'])