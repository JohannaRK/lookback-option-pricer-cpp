#ifndef GREEKS_CPP
#define GREEKS_CPP

#include "Greeks.hpp"
#include "BlackScholesModel.hpp"

#include <algorithm>
#include <cmath>

Greeks GreeksCalculator::computeGreeks_t0(const LookbackOption& option,
                                         const MonteCarloSimulator& mcBase,
                                         double t0,
                                         double z) const
{
    auto model = mcBase.getModel();

    const double S0    = model.getInitialPrice();
    const double sigma = model.getVolatility();
    const double r     = model.getRiskFreeRate();
    const double T     = option.getMaturity();

    // Spot: relative shock
    const double hS = std::max(epsS_ * std::abs(S0), 1e-4);

    // Volatility: relative shock
    const double hSigma = std::max(epsSigma_ * std::abs(sigma), 1e-4);

    // Rate: ABSOLUTE shock (e.g. 0.01 = 100 bps)
    const double hR = epsR_;

    // Time: absolute shock
    const double hT = epsT_;

    Greeks g;

    // -------------------------------------------------------------------------
    // PRICE (Control Variate) + (optional CI available in MCResult)
    // We store only the point estimate in Greeks.
    // -------------------------------------------------------------------------
    const auto baseRes = mcBase.simulateOptionPriceControlVariate_t0(option, t0, z);
    g.price = baseRes.price;

    // Store MC uncertainty for the price only
    g.priceStdError = baseRes.stdError;
    g.priceCILower  = baseRes.ciLower;
    g.priceCIUpper  = baseRes.ciUpper;
    // -------------------------------------------------------------------------
    // DELTA & GAMMA
    // -------------------------------------------------------------------------
    {
        BlackScholesModel mUp = model; mUp.setInitialPrice(S0 + hS);
        BlackScholesModel mDn = model; mDn.setInitialPrice(S0 - hS);

        MonteCarloSimulator mcUp(mUp, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());
        MonteCarloSimulator mcDn(mDn, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());

        const double pUp = mcUp.simulateOptionPriceControlVariate_t0(option, t0, z).price;
        const double pDn = mcDn.simulateOptionPriceControlVariate_t0(option, t0, z).price;

        g.delta = (pUp - pDn) / (2.0 * hS);
        g.gamma = (pUp - 2.0 * g.price + pDn) / (hS * hS);
    }

    // -------------------------------------------------------------------------
    // VEGA
    // -------------------------------------------------------------------------
    {
        BlackScholesModel mUp = model; mUp.setVolatility(sigma + hSigma);
        BlackScholesModel mDn = model; mDn.setVolatility(sigma - hSigma);

        MonteCarloSimulator mcUp(mUp, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());
        MonteCarloSimulator mcDn(mDn, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());

        const double pUp = mcUp.simulateOptionPriceControlVariate_t0(option, t0, z).price;
        const double pDn = mcDn.simulateOptionPriceControlVariate_t0(option, t0, z).price;

        g.vega = (pUp - pDn) / (2.0 * hSigma);
    }

    // -------------------------------------------------------------------------
    // RHO
    // -------------------------------------------------------------------------
    {
        BlackScholesModel mUp = model; mUp.setRiskFreeRate(r + hR);
        BlackScholesModel mDn = model; mDn.setRiskFreeRate(r - hR);

        MonteCarloSimulator mcUp(mUp, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());
        MonteCarloSimulator mcDn(mDn, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                 mcBase.getBatchSize(), mcBase.getSeed());

        const double pUp = mcUp.simulateOptionPriceControlVariate_t0(option, t0, z).price;
        const double pDn = mcDn.simulateOptionPriceControlVariate_t0(option, t0, z).price;

        g.rho = (pUp - pDn) / (2.0 * hR);
    }

    // -------------------------------------------------------------------------
    // THETA (one-sided, time-decay / current-time bump)
    // We want theta = ∂P/∂t. Since τ = T - t, we have ∂P/∂t = -∂P/∂τ.
    // Approximating a decrease of τ by hT is equivalent to reducing maturity T by hT
    // while keeping t0 fixed.
    // -------------------------------------------------------------------------
    {
        const double Tdn = std::max(T - hT, 1e-6);
        auto optDn = option.cloneWithMaturity(Tdn);

        MonteCarloSimulator mcSame(model, mcBase.getNumPaths(), mcBase.getNumSteps(),
                                   mcBase.getBatchSize(), mcBase.getSeed());

        const double pDn = mcSame.simulateOptionPriceControlVariate_t0(*optDn, t0, z).price;

        // theta = ∂P/∂t at t0 (time decay). With maturity fixed, increasing t reduces
        // time-to-maturity. Using a one-sided bump on time-to-maturity:
        //   theta ≈ (P(T - hT) - P(T)) / hT
        g.theta = (pDn - g.price) / hT;
    }

    return g;
}

#endif // GREEKS_CPP