#ifndef BLACKSCHOLESMODEL_HPP
#define BLACKSCHOLESMODEL_HPP
#include <vector>
#include <random>
#include <cmath>
class BlackScholesModel
{
    private:
        double riskFreeRate_; // Risk-free interest rate
        double volatility_;    // Volatility of the underlying asset
        double initialPrice_;  // Initial price of the underlying asset
    public:
        // Constructor
        BlackScholesModel(double r, double v, double S0)
            : riskFreeRate_(r), volatility_(v), initialPrice_(S0) {};
        // Getters
        double getRiskFreeRate() const { return riskFreeRate_; };
        double getVolatility() const { return volatility_; };
        double getInitialPrice() const { return initialPrice_; };
        // Setters
        void setRiskFreeRate(double r) { riskFreeRate_ = r; };
        void setVolatility(double v) { volatility_ = v; };
        void setInitialPrice(double S0) { initialPrice_ = S0; };
        // Black-Scholes analytical pricing formulas
        inline double priceCallBS(double S, double K, double T) const
        {
            double r = riskFreeRate_;
            double sigma = volatility_;

            double sqrtT = std::sqrt(T);
            double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
            double d2 = d1 - sigma * sqrtT;

            return S * 0.5 * std::erfc(-d1 / std::sqrt(2))
                 - K * std::exp(-r * T) * 0.5 * std::erfc(-d2 / std::sqrt(2));
        }

        inline double pricePutBS(double S, double K, double T) const
        {
            double r = riskFreeRate_;
            double sigma = volatility_;

            double sqrtT = std::sqrt(T);
            double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
            double d2 = d1 - sigma * sqrtT;

            return K * std::exp(-r * T) * 0.5 * std::erfc(d2 / std::sqrt(2))
                 - S * 0.5 * std::erfc(d1 / std::sqrt(2));
        }
        inline double simulate_step(double S, double Z,
                            double drift,
                            double volDt) const
        {
            return S * std::exp(drift + volDt * Z);
        }; 
        std::vector<double> simulate_path(double T, int N,
                                  std::mt19937& gen,
                                  std::normal_distribution<double>& dist) const;
        void simulate_path_into( std::vector<double>& path, double T,
                                int numSteps,
                                std::mt19937& gen,
                                std::normal_distribution<double>& dist) const;
};
#endif // BLACKSCHOLESMODEL_HPP