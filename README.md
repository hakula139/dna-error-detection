# dna-error-detection

[![wakatime](https://wakatime.com/badge/github/hakula139/dna-error-detection.svg)](https://wakatime.com/badge/github/hakula139/dna-error-detection)

An error detection algorithm for DNA sequences, written in modern C++.

- [Task 1 report](./report/task_1.md)
- [Task 2 report](./report/task_2.md)

## Getting Started

### Prerequisites

To set up the environment, you need to have the following dependencies installed.

- [GNU make](https://www.gnu.org/software/make) 4.0 or above
- [GCC](https://gcc.gnu.org/releases.html) 9.0 or above (for C++17 features)
  - GCC 8.0 or below and Clang 12.0 or below are **NOT** supported

### Building

- `make`: Build the project using GNU make, a Unix-like environment is required.

### Usages

- `make help`: Show arguments and usages.
- `make run`: Run all preprocessing tasks and start the main process.
- `make index`: Create an index of reference data only.
- `make minimizer`: Find minimizers only.
- `make start`: Find sv deltas only.

### Clean

- `make clean`: Remove all building files.
