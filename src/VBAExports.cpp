#include "VBAExports.h"

#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "LookbackOption.hpp"
#include "Greeks.hpp"

#include <limits>

namespace {

inline void fill_nan(double* out10) {
    if (!out10) return;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (int i = 0; i < 10; ++i) out10[i] = nan;
}

inline void fill_out10(const Greeks& g, double* out10) {
    if (!out10) return;
    out10[0] = g.price;
    out10[1] = g.delta;
    out10[2] = g.gamma;
    out10[3] = g.vega;
    out10[4] = g.rho;
    out10[5] = g.theta;
    out10[6] = g.priceStdError;
    out10[7] = g.priceCILower;
    out10[8] = g.priceCIUpper;
    out10[9] = 0.0;
}

inline int compute_all(bool isCall,
                       double S0, double r, double sigma, double T,
                       int numSteps, int numPaths, int batchSize, int seed,
                       double z,
                       double* out10)
{
    if (!out10) return -1;
    if (!(S0 > 0.0) || !(sigma > 0.0) || !(T > 0.0) || numSteps <= 0 || numPaths <= 0) {
        fill_nan(out10);
        return -2;
    }

    try {
        BlackScholesModel model(r, sigma, S0);
        MonteCarloSimulator mc(model, numPaths, numSteps, batchSize, seed);
        GreeksCalculator gc;

        if (isCall) {
            LookbackFloatingCallPayoff payoff;
            LookbackCallOption option(payoff, T);
            const Greeks g = gc.computeGreeks_t0(option, mc, /*t0=*/0.0, z);
            fill_out10(g, out10);
        } else {
            LookbackFloatingPutPayoff payoff;
            LookbackPutOption option(payoff, T);
            const Greeks g = gc.computeGreeks_t0(option, mc, /*t0=*/0.0, z);
            fill_out10(g, out10);
        }
        return 0;
    } catch (...) {
        fill_nan(out10);
        return -99;
    }
}

inline int compute_curve(bool isCall,
                         double spotStart, double spotStep, int nPoints,
                         double r, double sigma, double T,
                         int numSteps, int numPaths, int batchSize, int seed,
                         double z,
                         double* spots, double* prices, double* deltas)
{
    if (!spots || !prices || !deltas) return -1;
    if (nPoints <= 0) return -2;
    if (!(spotStep > 0.0) || !(spotStart > 0.0) || !(sigma > 0.0) || !(T > 0.0)) return -3;

    try {
        // IMPORTANT:
        // For the charts we only need PRICE and DELTA, not all Greeks.
        // Calling computeGreeks_t0 here would run many extra MC pricers per point
        // (delta/gamma/vega/rho/theta bumps). This version is ~8x faster.
        //
        // For floating-strike lookbacks in Black–Scholes, P(S0) is homogeneous of degree 1:
        //   P(S0) = S0 * C  =>  Delta = dP/dS0 = C = P(S0)/S0,  Gamma ≈ 0.
        // So we compute delta from the price estimate.

        if (isCall) {
            LookbackFloatingCallPayoff payoff;
            LookbackCallOption option(payoff, T);

            for (int i = 0; i < nPoints; ++i) {
                const double S0 = spotStart + static_cast<double>(i) * spotStep;
                spots[i] = S0;

                BlackScholesModel model(r, sigma, S0);

                // Use a different seed per point to avoid perfectly "rigid" straight lines
                // when reusing the exact same random numbers for every S0.
                const int seed_i = seed + i * 10007;

                MonteCarloSimulator mc(model, numPaths, numSteps, batchSize, seed_i);
                const auto res = mc.simulateOptionPriceControlVariate_t0(option, /*t0=*/0.0, z);

                prices[i] = res.price;
                deltas[i] = (S0 > 0.0) ? (res.price / S0) : 0.0;
            }
        } else {
            LookbackFloatingPutPayoff payoff;
            LookbackPutOption option(payoff, T);

            for (int i = 0; i < nPoints; ++i) {
                const double S0 = spotStart + static_cast<double>(i) * spotStep;
                spots[i] = S0;

                BlackScholesModel model(r, sigma, S0);
                const int seed_i = seed + i * 10007;

                MonteCarloSimulator mc(model, numPaths, numSteps, batchSize, seed_i);
                const auto res = mc.simulateOptionPriceControlVariate_t0(option, /*t0=*/0.0, z);

                prices[i] = res.price;
                deltas[i] = (S0 > 0.0) ? (res.price / S0) : 0.0;
            }
        }

        return 0;
    } catch (...) {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        for (int i = 0; i < nPoints; ++i) {
            spots[i]  = nan;
            prices[i] = nan;
            deltas[i] = nan;
        }
        return -99;
    }
}

} // namespace


int LB_CALLCONV Lookback_CallAll(
    double S0, double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* out10)
{
    return compute_all(true, S0, r, sigma, T, numSteps, numPaths, batchSize, seed, z, out10);
}

int LB_CALLCONV Lookback_PutAll(
    double S0, double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* out10)
{
    return compute_all(false, S0, r, sigma, T, numSteps, numPaths, batchSize, seed, z, out10);
}


int LB_CALLCONV Lookback_CallCurve(
    double spotStart, double spotStep, int nPoints,
    double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* spots, double* prices, double* deltas)
{
    return compute_curve(true, spotStart, spotStep, nPoints,
                         r, sigma, T,
                         numSteps, numPaths, batchSize, seed,
                         z,
                         spots, prices, deltas);
}

int LB_CALLCONV Lookback_PutCurve(
    double spotStart, double spotStep, int nPoints,
    double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* spots, double* prices, double* deltas)
{
    return compute_curve(false, spotStart, spotStep, nPoints,
                         r, sigma, T,
                         numSteps, numPaths, batchSize, seed,
                         z,
                         spots, prices, deltas);
}
