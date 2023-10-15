import utils

length = 10_000
# index = length // 4
index = 0

py_timer = utils.Timer(
    stmt=f"d[{index}] = None",
    setup=f"""
import collections
d = collections.deque()
for _ in range({length}): 
    d.append(None)
"""
)

my_timer = utils.Timer(
    stmt=f"d[{index}] = None",
    setup=f"""
import deque_one
d = deque_one.deque()
for _ in range({length}): 
    d.append(None)
"""
)


if __name__ == "__main__":
    repeat = 10
    number = 1_000_000
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")