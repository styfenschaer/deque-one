import utils

length = 1_000_000

py_timer = utils.Timer(
    stmt="d.pop()",
    setup=f"import collections; d = collections.deque([0]*{length})"
)

my_timer = utils.Timer(
    stmt="d.pop()",
    setup=f"import deque_one; d = deque_one.deque([0]*{length})"
)


if __name__ == "__main__":
    repeat = 10
    number = 1_000_000
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")