# Lookback Option Pricer in C++ (Monte Carlo with Excel/VBA Interface)

## Overview

This project implements the pricing of European floating-strike lookback options under the Black–Scholes model using Monte Carlo simulation in C++.  
The project also includes an Excel/VBA interface for parameter input, result display, and graphical visualization.

The implementation provides:
- theoretical option price,
- Delta, Gamma, Theta, Rho, and Vega,
- price curve as a function of the initial underlying price,
- Delta curve as a function of the initial underlying price.

## Project context

This project was carried out as part of the M2 Quantitative Finance curriculum.  
It focuses on the numerical pricing of path-dependent derivatives, with particular attention to software structure, reproducibility, and interface design.

## Main features

- Monte Carlo simulation under the Black–Scholes framework
- Pricing of European floating-strike lookback call and put options
- Computation of standard Greeks
- Excel/VBA interface for user interaction
- Graphical outputs for price and Delta
- Modular C++ implementation with separated headers and source files
- Documentation generated with Doxygen

## Repository structure

- `include/` : header files for the pricing model, Monte Carlo engine, option definitions, Greeks, and VBA exports  
- `src/` : C++ source files  
- `excel/` : Excel/VBA interface  
- `screenshots/` : illustrations of the Excel dashboard and output sheets  
- `test/` : test files  
- `docs/` : generated project documentation  
- `Makefile` : build configuration  
- `Doxyfile` : Doxygen configuration  

## Model and methodology

The pricing framework is based on Monte Carlo simulation in the Black–Scholes model.  
Since lookback options are path-dependent, the payoff depends on the minimum or maximum value reached by the underlying asset over the life of the option.

The project considers:
- floating-strike lookback calls,
- floating-strike lookback puts.

In addition to the option price, the implementation computes the main sensitivities and provides graphical representations with respect to the initial spot price.

## Interface

The user interface is implemented in Excel/VBA.  
It allows the user to enter model and simulation parameters, run the pricing engine, and visualize the resulting outputs.

## Tools

- C++
- Excel / VBA
- Monte Carlo simulation
- Doxygen

## Notes

This repository corresponds to the original project files used for the course, kept in their initial structure.
