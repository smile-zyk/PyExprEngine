import math
print(dir(math))
# 过滤掉以双下划线开头和结尾的内置属性，只看函数和常量
math_contents = [item for item in dir(math) if not item.startswith('_')]
print(math_contents)
print(f"Total number of functions and constants: {len(math_contents)}")