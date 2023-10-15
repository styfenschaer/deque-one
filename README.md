# deque-one
[![PyPI version](https://img.shields.io/pypi/v/deque-one?color=%2347ccbd)](https://pypi.org/project/deque-one/)
[![License](https://img.shields.io/pypi/l/deque-one?color=%2347ccbd)](https://opensource.org/licenses/BSD-3-Clause)
[![python](https://img.shields.io/pypi/pyversions/deque-one?color=%2347ccbd)](https://pypi.org/project/deque-one/)
[![status](https://img.shields.io/pypi/status/deque-one?color=%2347ccbd)](https://pypi.org/project/deque-one/)
[![downloads](https://img.shields.io/pypi/dm/deque-one?color=%2347ccbd)](https://pypi.org/project/deque-one/)

*Work-in-progress* drop-in replacement for Python's [`collection.deque`](https://docs.python.org/3/library/collections.html#collections.deque) with O(1) item access and without sacrificing performance on other metrics.

## Getting Started
The easiest way to get deque-one is to:
```
$ pip install deque-one
```
Alternatively, you can build it from source:
```
$ git clone https://github.com/styfenschaer/deque-one.git
$ cd deque-one
$ python setup.py install
``` 
The latter requires a C compiler compatible with your Python installation.

Once installed, it behaves like the built-in `deque` but with O(1) instead of O(n) random item access:
```python
import collections
import deque_one

col_deque = collections.deque(range(1_000_000))
one_deque = deque_one.deque(range(1_000_000))

%timeit col_deque[0]
%timeit one_deque[0]
# 33.6 ns ± 0.422 ns per loop (mean ± std. dev. of 7 runs, 10,000,000 loops each)
# 33.3 ns ± 1.08 ns per loop (mean ± std. dev. of 7 runs, 10,000,000 loops each)

%timeit col_deque[500_000]
%timeit one_deque[500_000]
# 89.3 µs ± 1.93 µs per loop (mean ± std. dev. of 7 runs, 10,000 loops each)
# 38.2 ns ± 1.45 ns per loop (mean ± std. dev. of 7 runs, 10,000,000 loops each)
```

## Implemented Methods
- `__init__` (only first argument)
- `append`
- `appendleft`
- `pop`
- `popleft`
- `extend`
- `extendleft`
- `clear`
- `__getitem__`
- `__setitem__`
- `__len__`
- `__repr__`
- `__str__`

## Implemented Properties
- `maxlen` (always `None`)

