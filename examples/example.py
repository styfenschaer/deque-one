import collections
import timeit
import time 
import deque_one

col_deque = collections.deque(range(1_000_000))
one_deque = deque_one.deque(range(1_000_000))

time.sleep(2.0)
print(timeit.timeit("col_deque[0]", number=100_000_000, globals=globals()))

time.sleep(2.0)
print(timeit.timeit("one_deque[0]", number=100_000_000, globals=globals()))

time.sleep(2.0)
print(timeit.timeit("col_deque[500_000]", number=100_000, globals=globals()))

time.sleep(2.0)
print(timeit.timeit("one_deque[500_000]", number=100_000, globals=globals()))