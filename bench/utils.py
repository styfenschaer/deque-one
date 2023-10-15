import time
import timeit 
import gc
from contextlib import contextmanager
from statistics import median, stdev
from rich import console
    
    
@contextmanager
def collect_and_sleep(seconds=2.0, generation=2):
    gc.collect(generation)
    time.sleep(seconds)
    yield 
    

class Timer(timeit.Timer):
    def __init__(self, stmt="pass", setup="pass", globals=None):
        self.timings = None
        super().__init__(stmt, setup, time.perf_counter_ns, globals)
    
    def exec(self, repeat: int, number: int):
        with collect_and_sleep():
            timings = self.repeat(repeat, number)
        self.timings = [t / number for t in timings]
        return self

    def print_stats(self, case: str):
        c = console.Console()
        c.print(case, style="blue")
        c.print(f"     min [ns]: {min(self.timings):.3f}", style="green")
        c.print(f"     max [ns]: {max(self.timings):.3f}", style="green")
        c.print(f"  median [ns]: {median(self.timings):.3f}", style="green")
        c.print(f"     std [ns]: {stdev(self.timings):.3f}", style="green")