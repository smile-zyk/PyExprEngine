import math
t = 2
class SimpleTest:
    
    def __init__(self, name="Test"):
        self.name = name
    
    def greet(self):
        return f"Hello, {self.name}!"
    
    def add(self, a, b):
        return a + b + t

def add(a, b):
    global s
    s = 1
    return a + b + s
a = SimpleTest()
print(a.greet())
print(a.add(3, 4))
print(add(5, 6))
print(t)
print(s)