import utils


py_timer = utils.Timer(
    stmt="deque()",
    setup="from collections import deque",
)

my_timer = utils.Timer(
    stmt="deque()",
    setup="from deque_one import deque",
)


if __name__ == "__main__":
    repeat = 10
    number = 100_000
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")