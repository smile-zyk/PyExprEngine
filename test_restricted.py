from RestrictedPython import compile_restricted

def check_code_safety(code_string):
    """
    静态检查代码是否满足 RestrictedPython 的安全限制。
    返回 (is_safe: bool, message: str)
    """
    try:
        # 尝试编译代码
        compile_restricted(code_string, filename='<check>', mode='exec')
        return True, "代码通过安全检查"
    except SyntaxError as e:
        # 捕获语法错误或违反限制的错误（如出现 import, exec 等）
        return False, f"安全检查失败: {e.msg} (at line {e.lineno})"
    except Exception as e:
        # 捕获其他意外错误
        return False, f"编译过程发生错误: {str(e)}"
    
# 测试用例
test_cases = [
    # Case 1: 安全的数学运算
    """
x = 1 + 2 * 3
result = x / 2
    """,
    
    # Case 2: 危险的 import 语句 -> 会被捕获
    """
import os
os.system('rm -rf /')
    """,
    
    # Case 3: 危险的 exec 语句 -> 会被捕获
    """
exec('print("dangerous")')
    """,
    
    # Case 4: 尝试访问私有属性 -> 编译可能成功，但运行时会失败
    """
class Test:
    def __init__(self):
        self._private = 42
obj = Test()
value = obj._private # 这里编译通过，但执行时可能被策略拦截
    """
]

for i, code in enumerate(test_cases, 1):
    is_safe, message = check_code_safety(code)
    print(f"用例 {i}: {'通过' if is_safe else '失败'} - {message}")