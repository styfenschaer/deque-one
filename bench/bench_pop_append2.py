import utils

py_timer = utils.Timer(
    stmt="""
d.append(None)
d.appendleft(None)
d.popleft()
d.pop()
""", setup="import collections ; d = collections.deque()",
)

my_timer = utils.Timer(
    stmt="""
d.append(None)
d.appendleft(None)
d.popleft()
d.pop()
""", setup="import deque_one; d = deque_one.deque()",
)


if __name__ == "__main__":
    repeat = 10
    number = 200_000
    
    py_timer.exec(repeat=repeat, number=number).print_stats(case="Python")
    my_timer.exec(repeat=repeat, number=number).print_stats(case="Custom")
