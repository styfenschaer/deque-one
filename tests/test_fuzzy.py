import deque_one
import collections
from time import perf_counter
import random
from rich import console
import numpy as np


def check_all_equal(iterable, callback=lambda o: o):
    iterable = list(iterable)
    if not iterable:
        return True

    item0 = callback(iterable[0])
    for item in iterable:
        if callback(item) != item0:
            return False
    return True


sentinel = type("_Sentinel", (), {})


class FuzzyTest:
    def __init__(self, *objects, callbacks: None, seed=None):
        self.objects = objects
        self.options = []
        self.probs = []
        self.assertions = []
        self.callbacks = callbacks or []
        self.rng = np.random.default_rng(seed)

        self.latest_name = None
        self.latest_arggen = None
        self.latest_args = sentinel
        self.latest_results = None
        self.latest_exceptions = None

    def execute(self, duration_seconds=2):
        probs = [p / sum(self.probs) for p in self.probs]

        count = 0
        tic = perf_counter()
        while (elapsed := perf_counter() - tic) < duration_seconds:
            name, arggen = self.rng.choice(self.options, p=probs)
            self._execute(name, arggen)

            for callback in self.callbacks:
                callback(self)
            count += 1

        c = console.Console()
        c.print(f"Completed {count:,} tests in {elapsed:.2f} sec.", style="green")

    def register_option(self, name, arggen, prob=None):
        self.options.append((name, arggen))
        self.probs.append(1.0 if prob is None else prob)

    def register_assertion(self, assertion):
        self.assertions.append(assertion)

    def _check_equal_exceptions(self, exceptions):
        if not exceptions:
            return

        if len(exceptions) != len(self.objects):
            raise ValueError("only some objects raised expection")

        exciter = iter(exceptions)
        exc0 = next(exciter)
        for exc in exciter:
            if type(exc) is not type(exc0):
                raise TypeError(f"unequal expection {exc0} and {exc}")

    def _execute(self, name: str, arggen):
        args = arggen()
        results, exceptions = [], []
        for obj in self.objects:
            try:
                results.append(getattr(obj, name)(*args))
            except Exception as exc:
                exceptions.append(exc)

        self.latest_name = name
        self.latest_arggen = arggen
        self.latest_args = args
        self.latest_results = results
        self.latest_exceptions = exceptions

        self._check_equal_exceptions(exceptions)

        for assertion in self.assertions:
            assertion(self)


class MyRandnumGenerator:
    def __init__(self, seed: int):
        self.rng = random.Random(seed)

    def rand(self):
        return (self.rng.random(),)

    def rand_multiple(self):
        n = self.rng.randint(0, 100)
        return ([self.rng.random() for _ in range(n)],)

    def get_int(self):
        return (self.rng.randint(-1_000, 1_000),)


class DequeAssertions:
    def __call__(self, fuzzy_test: FuzzyTest):
        match fuzzy_test.latest_name:
            case "append" | "appendleft":
                assert check_all_equal(fuzzy_test.objects, callback=list)
            case "extend" | "extendleft":
                assert check_all_equal(fuzzy_test.objects, callback=list)
            case "pop" | "popleft":
                assert check_all_equal(fuzzy_test.latest_results)
                assert check_all_equal(fuzzy_test.objects, callback=list)
            case "clear":
                assert check_all_equal(fuzzy_test.objects, callback=list)
            case "__setitem__":
                assert check_all_equal(fuzzy_test.objects, callback=list)
            case "__getitem__":
                assert check_all_equal(fuzzy_test.latest_results)
            case "__str__" | "__repr__":
                assert check_all_equal(fuzzy_test.latest_results)
            case "__len__":
                assert check_all_equal(fuzzy_test.latest_results)
            case _:
                assert False, "unreachable"


class LengthCallback:
    def __init__(self):
        self.lengths: list[int] = []

    def __call__(self, fuzzy_test: FuzzyTest):
        self.lengths.append(len(fuzzy_test.objects[0]))

    def plot_lengths(self):
        import matplotlib.pyplot as plt

        plt.figure()
        plt.hist(self.lengths, bins="scott", alpha=0.5)
        plt.show()


if __name__ == "__main__":
    rand = MyRandnumGenerator(42)
    assertion = DequeAssertions()
    length_callback = LengthCallback()

    py_deque = collections.deque()
    my_deque = deque_one.deque()

    fuzzy_test = FuzzyTest(py_deque, my_deque, callbacks=[length_callback])
    fuzzy_test.register_option(name="append", arggen=rand.rand, prob=1.0)
    fuzzy_test.register_option(name="appendleft", arggen=rand.rand, prob=1.0)
    fuzzy_test.register_option(name="pop", arggen=tuple, prob=1.0)
    fuzzy_test.register_option(name="popleft", arggen=tuple, prob=1.0)
    fuzzy_test.register_option(name="extend", arggen=rand.rand_multiple, prob=0.01)
    fuzzy_test.register_option(name="extendleft", arggen=rand.rand_multiple, prob=0.01)
    fuzzy_test.register_option(name="clear", arggen=tuple, prob=0.001)
    fuzzy_test.register_option(name="__getitem__", arggen=rand.get_int, prob=0.1)
    fuzzy_test.register_option(name="__setitem__", arggen=rand.get_int, prob=0.1)
    fuzzy_test.register_option(name="__len__", arggen=tuple, prob=0.001)
    fuzzy_test.register_option(name="__str__", arggen=tuple, prob=0.001)
    fuzzy_test.register_option(name="__repr__", arggen=tuple, prob=0.001)
    fuzzy_test.register_assertion(assertion)
    fuzzy_test.execute(duration_seconds=300.0)

    # length_callback.plot_lengths()
