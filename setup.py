import pathlib
import re

from setuptools import Extension, find_packages, setup


def get_version(rel_path):
    with open(pathlib.Path(__file__).parent / rel_path) as file:
        return re.search(r'__version__ = "(.*?)"', file.read())[1]


with open("README.md") as file:
    long_description = file.read()


setup(
    name="deque-one",
    version=get_version("deque_one/_version.py"),
    description="like collections.deque, but different",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Styfen Schär",
    author_email="styfen.schaer.blog@gmail.com",
    url="https://github.com/styfenschaer/deque-one",
    download_url="https://github.com/styfenschaer/deque-one",
    packages=find_packages(),
    include_package_data=True,
    package_data={"deque_one": ["*.pyi"]},
    ext_modules=[
        Extension(
            name="deque_one.deque",
            sources=["deque_one/_deque.c"],
        ),
    ],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: BSD License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
    ],
    keywords=["deque", "collections", "data-structure", "python"],
    package_dir={"deque_one": "deque_one"},
    extras_require={
        "dev": ["rich", "numpy"],
    },
)
