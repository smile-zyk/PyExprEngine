from RestrictedPython import compile_restricted, safe_builtins
from RestrictedPython.Eval import default_guarded_getitem, default_guarded_getiter
from RestrictedPython.Guards import guarded_unpack_sequence, guarded_iter_unpack_sequence, full_write_guard
import RestrictedPython

# 创建一个基础的安全全局变量字典
safe_globals = {
    '__builtins__': safe_builtins, # 这是 RestrictedPython 提供的受限内置函数集合
    '_getiter_': default_guarded_getiter,
    '_getitem_': default_guarded_getitem,
    '_unpack_sequence_': guarded_unpack_sequence,
    '_iter_unpack_sequence_': guarded_iter_unpack_sequence,
    '_write_guard_': full_write_guard,
    'sum' : sum
}

untrusted_code = """
result = []
for i in range(5):
    result.append(i * i)
output = sum(result)
"""

# 创建一个本地命名空间来捕获执行结果
local_ns = {}

# 1. 编译代码
byte_code = compile_restricted(untrusted_code, filename='<untrusted>', mode='exec')

# 2. 执行代码，传入安全的全局变量和本地命名空间
exec(byte_code, safe_globals, local_ns)

# 3. 从本地命名空间中获取结果
print(local_ns['output']) # 输出： 30 (0+