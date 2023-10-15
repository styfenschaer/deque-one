import utils

length = 100_000
value = 42 

py_timer = utils.Timer(
    stmt=f"d.count({value})",
    setup=f"""
import collections
import random
nums = [random.randint(0, 100) for _ in range({length})]
d = collections.deque(nums)
""",
)

my_timer = utils.Timer(
    stmt=f"d.count({value})",
    setup=f"""
import deque_one
import random
nums = [random.randint(0, 100) for _ in range({length})]
d = deque_one.deque(nums)
"""
)


if __name__ == "__main__":
    raise NotImplementedError

    repeat = 10
    number = 100
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")
