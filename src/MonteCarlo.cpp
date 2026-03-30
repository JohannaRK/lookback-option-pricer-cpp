#ifndef MONTECARLO_CPP
#define MONTECARLO_CPP

#include "MonteCarlo.hpp"
#include <omp.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <vector>
#include <random>
#include <memory>

// CHANGED SIGNATURE: return MCResult + add z parameter
MonteCarloSimulator::MCResult
MonteCarloSimulator::simulateOptionPriceControlVariate_t0(const LookbackOption& option, double t0, double z) const
{
    const int nThreads = omp_get_max_threads();

    // Thread-local PRNGs
    std::vector<std::mt19937> generators(nThreads);
    for (int tid = 0; tid < nThreads; ++tid)
        generators[tid] = std::mt19937(seed + tid * 1000003);

    // Per-thread global accumulators (written once after the loop)
    std::vector<double> threadSumX(nThreads, 0.0);
    std::vector<double> threadSumY(nThreads, 0.0);
    std::vector<double> threadSumY2(nThreads, 0.0);
    std::vector<double> threadSumXY(nThreads, 0.0);

    // ADDED: needed for Var(X)
    std::vector<double> threadSumX2(nThreads, 0.0);

    // Vanilla payoff (polymorphic)
    std::unique_ptr<Payoff> vanillaPayoff;
    if (option.isCall()) vanillaPayoff = std::make_unique<VanillaCallPayoff>();
    else                vanillaPayoff = std::make_unique<VanillaPutPayoff>();

    // Cache lookback payoff (virtual dispatch)
    const Payoff& lbPayoff = option.getPayoff();

    // Model / option parameters
    const double r = model_.getRiskFreeRate();
    const double T = option.getMaturity();

    const double S0 = model_.getInitialPrice();
    const double K  = S0 * std::exp(r * (T - t0));

    const double C0 = option.isCall()
        ? model_.priceCallBS(S0, K, T - t0)
        : model_.pricePutBS(S0, K, T - t0);

    const double disc = std::exp(-r * (T - t0));

    const int N  = numPaths_;
    const int bs = batchSize_;

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        auto& gen = generators[tid];
        std::normal_distribution<double> dist(0.0, 1.0);

        // Thread-local preallocation (avoid reallocations/resizes)
        std::vector<double> path(numSteps_ + 1);
        std::vector<double> strikes(numSteps_ + 1);

        double localSumX  = 0.0;
        double localSumY  = 0.0;
        double localSumY2 = 0.0;
        double localSumXY = 0.0;

        // ADDED: local X^2 accumulator
        double localSumX2 = 0.0;

        #pragma omp for schedule(static)
        for (int i = 0; i < N; i += bs)
        {
            const int end = std::min(i + bs, N);
            for (int j = i; j < end; ++j)
            {
                model_.simulate_path_into(path, T, numSteps_, gen, dist);
                const double ST = path[numSteps_];

                option.computePathStrikesInto(path, strikes);
                const double strike0 = strikes[0];

                const double X = disc * lbPayoff(ST, strike0);
                const double Y = disc * (*vanillaPayoff)(ST, K);

                localSumX  += X;
                // ADDED: accumulate X^2
                localSumX2 += X * X;

                localSumY  += Y;
                localSumY2 += Y * Y;
                localSumXY += X * Y;
            }
        }

        // Single writeback per thread
        threadSumX[tid]  += localSumX;
        // ADDED: writeback X^2
        threadSumX2[tid] += localSumX2;

        threadSumY[tid]  += localSumY;
        threadSumY2[tid] += localSumY2;
        threadSumXY[tid] += localSumXY;
    }

    // Reduction across threads
    double sumX = 0.0, sumY = 0.0, sumY2 = 0.0, sumXY = 0.0;
    // ADDED: reduce X^2
    double sumX2 = 0.0;

    for (int tid = 0; tid < nThreads; ++tid) {
        sumX  += threadSumX[tid];
        sumX2 += threadSumX2[tid];   // ADDED
        sumY  += threadSumY[tid];
        sumY2 += threadSumY2[tid];
        sumXY += threadSumXY[tid];
    }

    const double invN = 1.0 / static_cast<double>(numPaths_);
    const double meanX  = sumX  * invN;
    const double meanY  = sumY  * invN;
    const double meanY2 = sumY2 * invN;
    const double meanXY = sumXY * invN;

    // ADDED: mean of X^2 and Var(X)
    const double meanX2 = sumX2 * invN;

    const double varY  = meanY2 - meanY * meanY;
    const double covXY = meanXY - meanX * meanY;

    // ADDED: Var(X)
    const double varX  = meanX2 - meanX * meanX;

    const double alpha = (varY > 0.0) ? (covXY / varY) : 0.0;

    // CHANGED: compute price then CI
    const double price = meanX - alpha * (meanY - C0);

    // Plug-in variance of Z = X - alpha*(Y - C0) with alpha treated as fixed
    const double varZ = varX + alpha * alpha * varY - 2.0 * alpha * covXY;

    const double N_d = static_cast<double>(numPaths_);
    const double stdError = (varZ > 0.0) ? std::sqrt(varZ / N_d) : 0.0;

    const double ciLower = price - z * stdError;
    const double ciUpper = price + z * stdError;

    return MCResult{price, stdError, ciLower, ciUpper};
}

#endif