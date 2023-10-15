import utils

length = 100_000

py_timer = utils.Timer(
    stmt=f"d.index(1)",
    setup=f"""
import collections
d = collections.deque([0] * {length} + [1])
""",
)

my_timer = utils.Timer(
    stmt=f"d.index(1)",
    setup=f"""
import deque_one
d = deque_one.deque([0] * {length} + [1])
"""
)


if __name__ == "__main__":
    raise NotImplementedError

    repeat = 10
    number = 100
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")
