#ifndef GREEKS_HPP
#define GREEKS_HPP
#include "BlackScholesModel.hpp"
#include <cmath>
#include "MonteCarlo.hpp"
#include "LookbackOption.hpp"
#include <vector>

struct Greeks {
    double price = 0.0;

    // MC uncertainty (price only)
    double priceStdError = 0.0;
    double priceCILower  = 0.0;
    double priceCIUpper  = 0.0;

    double delta = 0.0;
    double gamma = 0.0;
    double vega  = 0.0;
    double rho   = 0.0;
    double theta = 0.0;
};

class GreeksCalculator {
private:
    double epsS_     = 1e-3;
    double epsSigma_ = 1e-2;
    double epsR_     = 1e-2;
    double epsT_     = 1.0/365.0;

public:
    GreeksCalculator() = default;
    GreeksCalculator(double epsS, double epsSigma, double epsR, double epsT)
        : epsS_(epsS), epsSigma_(epsSigma), epsR_(epsR), epsT_(epsT) {}

    Greeks computeGreeks_t0(const LookbackOption& option,
                        const MonteCarloSimulator& mcBase,
                        double t0,
                        double z) const;
};
#endif // GREEKS_HPP