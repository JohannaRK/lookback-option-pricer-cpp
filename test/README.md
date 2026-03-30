## Test Suite for Lookback Pricer (Boost.Test)

This project provides a robust **unit test suite** to validate the C++ core of **Projet_Informatique_M2QF-main** (Monte-Carlo pricing of European lookback options under Black–Scholes).
The tests rely on **Boost.Test** to ensure correctness, numerical sanity, and reproducibility of the implemented algorithms.

### What we test

* **Black–Scholes formulas** (vanilla **put–call parity**)
* **Path simulation** (vector size, positivity, finite values, exceptions on invalid inputs)
* **Lookback logic** (running **min/max** strike conventions)
* **Monte-Carlo + control variate** (confidence interval consistency, determinism in single-thread mode)
* **Greeks** (homogeneity property: price ∝ S0 ⇒ **delta = price/S0**, **gamma ≈ 0**)

---

#### Prerequisites

To set up the environment, make sure the following dependencies are installed:

* **Boost.Test**
* **C++ compiler supporting C++17** (e.g., `g++`)
* **OpenMP** support (used by the project)

On WSL / Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y g++ libboost-test-dev
```

> Note: We compile with `-fopenmp` because the Monte-Carlo engine uses OpenMP.

---

### Compilation

Compile the test suite **from the repository root** (`Projet_Informatique_M2QF-main/`) :

```bash
g++ -std=c++17 -O2 -Iinclude -fopenmp \
  test/tests_lookback.cpp \
  src/BlackScholesModel.cpp src/MonteCarlo.cpp src/Greeks.cpp \
  -o test/test_lookback
```

---

### Usage

Run the tests:

```bash
./test/test_lookback
```

---

### Important Notes

* Source files are located in `src/` and headers in `include/`.
* The test suite uses the **C++17** standard.
* For **reproducibility**, some tests force **single-thread execution** (`omp_set_num_threads(1)`) and use a **fixed RNG seed**.
