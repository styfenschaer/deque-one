import utils

py_timer = utils.Timer(
    stmt="d.extend(l)",
    setup="import collections ; d = collections.deque(); l = [None]*100",
)

my_timer = utils.Timer(
    stmt="d.extend(l)",
    setup="import deque_one; d = deque_one.deque(); l = [None]*100",
)


if __name__ == "__main__":
    repeat = 10
    number = 10_000
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")
