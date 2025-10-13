import builtins
from RestrictedPython import safe_globals

print("Python原生builtin函数:")
print("=" * 50)

# 获取原生builtin的所有属性
native_builtins = dir(builtins)
native_callables = []

for name in native_builtins:
    if not name.startswith('_'):  # 过滤掉内部属性
        obj = getattr(builtins, name)
        if callable(obj):
            native_callables.append(name)

# 打印原生builtin函数
for func_name in sorted(native_callables):
    print(func_name)

print("=" * 50)
print(f"Python原生builtin函数总数: {len(native_callables)}")

print("\n" + "=" * 80)

# 获取RestrictedPython safe_globals中的builtin函数
safe_builtins_dict = safe_globals.get('__builtins__', {})
if hasattr(safe_builtins_dict, '__dict__'):
    safe_builtins_dict = safe_builtins_dict.__dict__

safe_callables = []
for key, value in safe_builtins_dict.items():
    if callable(value) and not key.startswith('_'):
        safe_callables.append(key)

print("RestrictedPython safe_globals中的builtin函数:")
print("=" * 50)
for func_name in sorted(safe_callables):
    print(func_name)

print("=" * 50)
print(f"RestrictedPython safe_globals builtin函数总数: {len(safe_callables)}")

print("\n" + "=" * 80)

# 对比分析
print("对比分析:")
print("=" * 50)
print(f"Python原生builtin函数: {len(native_callables)} 个")
print(f"RestrictedPython safe_globals: {len(safe_callables)} 个")
print(f"被限制的函数: {len(native_callables) - len(safe_callables)} 个")

# 找出被限制的函数
native_set = set(native_callables)
safe_set = set(safe_callables)
restricted_functions = native_set - safe_set

print("\n被限制的危险函数:")
print("=" * 30)
for func_name in sorted(restricted_functions):
    print(func_name)

print("\n安全的builtin函数:")
print("=" * 30)
for func_name in sorted(safe_set):
    print(func_name)