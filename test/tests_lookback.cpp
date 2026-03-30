// ================================
// LookbackPricerTests (Boost.Test)
// We validate key building blocks of the project:
// - Black–Scholes vanilla formulas (put–call parity)
// - GBM path simulation sanity + input validation
// - Lookback strike logic (running min/max convention used in our code)
// - Monte-Carlo outputs (CI consistency, determinism with fixed seed + 1 thread, homogeneity)
// - Greeks structural properties (homogeneity => delta = price/S0 and gamma ~ 0)
// ================================

#define BOOST_TEST_MODULE LookbackPricerTests
#include <boost/test/included/unit_test.hpp>

#include <cmath>
#include <vector>
#include <memory>
#include <limits>
#include <omp.h>

// We include our project headers (pricing engine)
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "LookbackOption.hpp"
#include "Greeks.hpp"

// We use a tiny helper to ensure numbers are not NaN/Inf.
static bool is_finite(double x) {
    return std::isfinite(x);
}

// ======================================================
// 1) BLACK_SCHOLES tests: validate basic analytical pieces
// ======================================================
BOOST_AUTO_TEST_SUITE(BLACK_SCHOLES)

// We check put–call parity for vanilla options:
//
//   C - P = S - K * exp(-rT)
//
// If our call/put formulas are correct, this identity must hold.
BOOST_AUTO_TEST_CASE(PutCallParity) {
    const double S = 100.0;
    const double K = 110.0;
    const double r = 0.03;
    const double sigma = 0.25;
    const double T = 1.5;

    BlackScholesModel m(r, sigma, S);

    const double call = m.priceCallBS(S, K, T);
    const double put  = m.pricePutBS(S, K, T);

    const double lhs = call - put;
    const double rhs = S - K * std::exp(-r * T);

    // BOOST_CHECK_CLOSE uses relative error in percent.
    BOOST_CHECK_CLOSE(lhs, rhs, 1e-8); // in percent
}

// We verify that path simulation returns the right size (N steps => N+1 points),
// stays strictly positive (GBM property), and values are finite.
BOOST_AUTO_TEST_CASE(SimulatePathSizeAndPositive) {
    const double S0 = 100.0;
    BlackScholesModel m(0.05, 0.2, S0);

    // We fix the RNG seed to make the test reproducible.
    std::mt19937 gen(42);
    std::normal_distribution<double> dist(0.0, 1.0);

    const int N = 252;
    auto path = m.simulate_path(1.0, N, gen, dist);

    // We expect the vector to include S0 + N simulated points.
    BOOST_REQUIRE_EQUAL(path.size(), (size_t)N + 1);
    for (double s : path) {
        BOOST_CHECK(s > 0.0);
        BOOST_CHECK(is_finite(s));
    }
}

// We validate input checking: a non-positive number of steps should throw.
BOOST_AUTO_TEST_CASE(SimulatePathThrowsOnNonPositiveSteps) {
    BlackScholesModel m(0.05, 0.2, 100.0);
    std::mt19937 gen(42);
    std::normal_distribution<double> dist(0.0, 1.0);

    BOOST_CHECK_THROW(m.simulate_path(1.0, 0, gen, dist), std::invalid_argument);
    BOOST_CHECK_THROW(m.simulate_path(1.0, -5, gen, dist), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

// ======================================================
// 2) LOOKBACK_STRIKES tests: validate running min/max logic
// ======================================================
BOOST_AUTO_TEST_SUITE(LOOKBACK_STRIKES)

// For a floating lookback CALL in our implementation,
// the "strike path" follows the running MIN convention used by the code.
// We test both vector strike generation and direct strike-at-index queries.
BOOST_AUTO_TEST_CASE(CallRunningMinStrikes) {
    LookbackFloatingCallPayoff payoff;
    LookbackCallOption opt(payoff, 1.0);

    std::vector<double> path = {100, 90, 120, 80, 110};

    auto strikes = opt.computePathStrikes(path);
    std::vector<double> expected = {80, 80, 80, 80, 110};

    BOOST_REQUIRE_EQUAL(strikes.size(), expected.size());
    for (size_t i = 0; i < strikes.size(); ++i)
        BOOST_CHECK_EQUAL(strikes[i], expected[i]);

    // We also check the scalar accessor computeStrike at a few indices.
    BOOST_CHECK_EQUAL(opt.computeStrike(path, 0), 80);
    BOOST_CHECK_EQUAL(opt.computeStrike(path, 4), 110);
    BOOST_CHECK_EQUAL(opt.computeStrike(path, 2), 80);
}

// For a floating lookback PUT in our implementation,
// the "strike path" follows the running MAX convention used by the code.
BOOST_AUTO_TEST_CASE(PutRunningMaxStrikes) {
    LookbackFloatingPutPayoff payoff;
    LookbackPutOption opt(payoff, 1.0);

    std::vector<double> path = {100, 90, 120, 80, 110};

    std::vector<double> strikes;
    opt.computePathStrikesInto(path, strikes);
    std::vector<double> expected = {120, 120, 120, 110, 110};

    BOOST_REQUIRE_EQUAL(strikes.size(), expected.size());
    for (size_t i = 0; i < strikes.size(); ++i)
        BOOST_CHECK_EQUAL(strikes[i], expected[i]);

    BOOST_CHECK_EQUAL(opt.computeStrike(path, 0), 120);
    BOOST_CHECK_EQUAL(opt.computeStrike(path, 3), 110);
}

BOOST_AUTO_TEST_SUITE_END()

// ======================================================
// 3) MONTE_CARLO tests: validate statistical outputs + invariants
// ======================================================
BOOST_AUTO_TEST_SUITE(MONTE_CARLO)

// We check that:
// - the MC result is finite,
// - stdError is non-negative,
// - CI brackets the point estimate,
// - CI width matches 2*z*stdError (symmetric normal CI as implemented).
BOOST_AUTO_TEST_CASE(CIContainsPriceAndHasCorrectWidth) {
    // We force 1 thread to make the result deterministic and avoid reduction noise.
    omp_set_num_threads(1);

    double S0 = 100, r = 0.05, sigma = 0.2, T = 1.0;
    int steps = 252, paths = 20000, batch = 4, seed = 42;
    double t0 = 0.0, z = 1.96;

    BlackScholesModel model(r, sigma, S0);

    LookbackFloatingCallPayoff payoff;
    LookbackCallOption option(payoff, T);

    MonteCarloSimulator mc(model, paths, steps, batch, seed);
    auto res = mc.simulateOptionPriceControlVariate_t0(option, t0, z);

    BOOST_CHECK(is_finite(res.price));
    BOOST_CHECK(res.stdError >= 0.0);
    BOOST_CHECK(res.ciLower <= res.price);
    BOOST_CHECK(res.price <= res.ciUpper);

    const double width = res.ciUpper - res.ciLower;
    BOOST_CHECK_CLOSE(width, 2.0 * z * res.stdError, 1e-10);
}

// We verify determinism when we run twice with the same seed and 1 thread.
// This is important for reproducible tests.
BOOST_AUTO_TEST_CASE(DeterministicWithSingleThreadAndSeed) {
    omp_set_num_threads(1);

    double S0 = 100, r = 0.05, sigma = 0.2, T = 1.0;
    int steps = 252, paths = 5000, batch = 4, seed = 123;
    double t0 = 0.0, z = 1.96;

    BlackScholesModel model(r, sigma, S0);

    LookbackFloatingPutPayoff payoff;
    LookbackPutOption option(payoff, T);

    MonteCarloSimulator mc(model, paths, steps, batch, seed);
    auto res1 = mc.simulateOptionPriceControlVariate_t0(option, t0, z);
    auto res2 = mc.simulateOptionPriceControlVariate_t0(option, t0, z);

    BOOST_CHECK_EQUAL(res1.price, res2.price);
    BOOST_CHECK_EQUAL(res1.stdError, res2.stdError);
    BOOST_CHECK_EQUAL(res1.ciLower, res2.ciLower);
    BOOST_CHECK_EQUAL(res1.ciUpper, res2.ciUpper);
}

// We test positive homogeneity in S0 for our lookback payoff under BS:
// if we scale S0 by a factor a, the whole path scales by a and the payoff scales by a,
// so the price should scale by a as well (same random numbers via same seed).
BOOST_AUTO_TEST_CASE(HomogeneityPriceScalesWithSpot) {
    omp_set_num_threads(1);

    double r = 0.05, sigma = 0.2, T = 1.0;
    int steps = 252, paths = 10000, batch = 4, seed = 42;
    double t0 = 0.0, z = 1.96;

    LookbackFloatingCallPayoff payoff;
    LookbackCallOption option(payoff, T);

    BlackScholesModel m1(r, sigma, 100.0);
    BlackScholesModel m2(r, sigma, 150.0);

    MonteCarloSimulator mc1(m1, paths, steps, batch, seed);
    MonteCarloSimulator mc2(m2, paths, steps, batch, seed);

    const double p1 = mc1.simulateOptionPriceControlVariate_t0(option, t0, z).price;
    const double p2 = mc2.simulateOptionPriceControlVariate_t0(option, t0, z).price;

    BOOST_CHECK_CLOSE(p2, 1.5 * p1, 1e-12);
}

BOOST_AUTO_TEST_SUITE_END()

// ======================================================
// 4) GREEKS tests: structural properties for this payoff/model
// ======================================================
BOOST_AUTO_TEST_SUITE(GREEKS)

// For a homogeneous payoff (degree 1 in S0), we should have:
// - price(S0) = S0 * price(1)
// - delta = dP/dS0 = price/S0
// - gamma = d2P/dS0^2 = 0
//
// We compute Greeks with our code and check these relationships.
// We also include optional regression checks with fixed seed + 1 thread.
BOOST_AUTO_TEST_CASE(DeltaEqualsPriceOverSpotAndGammaNearZero) {
    omp_set_num_threads(1);

    double S0 = 100, r = 0.05, sigma = 0.2, T = 1.0;
    int steps = 252, paths = 20000, batch = 4, seed = 42;
    double t0 = 0.0, z = 1.96;

    GreeksCalculator calc;

    // CALL
    {
        LookbackFloatingCallPayoff payoff;
        LookbackCallOption option(payoff, T);

        BlackScholesModel model(r, sigma, S0);
        MonteCarloSimulator mc(model, paths, steps, batch, seed);

        Greeks g = calc.computeGreeks_t0(option, mc, t0, z);

        BOOST_CHECK(is_finite(g.price));
        BOOST_CHECK(is_finite(g.delta));
        BOOST_CHECK(is_finite(g.gamma));

        // Homogeneity identity for this payoff:
        BOOST_CHECK_CLOSE(g.delta, g.price / S0, 1e-8);
        BOOST_CHECK_SMALL(g.gamma, 1e-8);

        // Optional: regression check (single-thread, fixed seed)
        BOOST_CHECK_CLOSE(g.price, 16.6381533494, 1e-6);
    }

    // PUT
    {
        LookbackFloatingPutPayoff payoff;
        LookbackPutOption option(payoff, T);

        BlackScholesModel model(r, sigma, S0);
        MonteCarloSimulator mc(model, paths, steps, batch, seed);

        Greeks g = calc.computeGreeks_t0(option, mc, t0, z);

        BOOST_CHECK_CLOSE(g.delta, g.price / S0, 1e-8);
        BOOST_CHECK_SMALL(g.gamma, 1e-8);

        // Optional: regression check (single-thread, fixed seed)
        BOOST_CHECK_CLOSE(g.price, 13.5141932120, 1e-6);
    }
}

BOOST_AUTO_TEST_SUITE_END()
