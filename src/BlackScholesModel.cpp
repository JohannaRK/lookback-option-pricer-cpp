#ifndef BLACKSCHOLESMODEL_CPP
#define BLACKSCHOLESMODEL_CPP
#include "BlackScholesModel.hpp"
#include <cmath>
#include <stdexcept>
std::vector<double> BlackScholesModel::simulate_path(double T, int N,
                                                     std::mt19937& gen,
                                                     std::normal_distribution<double>& dist) const
{
    if (N <= 0)
    {
        throw std::invalid_argument("Number of steps N must be positive.");
    }
    std::vector<double> path(N + 1);
    double dt = T / N;
    path[0] = initialPrice_;
    const double drift = (riskFreeRate_ - 0.5 * volatility_ * volatility_) * dt;
    const double volDt = volatility_ * std::sqrt(dt);
    for (int i = 0; i < N; ++i) 
    {
        double Z = dist(gen);
        path[i + 1] = simulate_step(path[i], Z, drift, volDt); // Could remove call to inline function for performance
    }   
    return path;
}
void BlackScholesModel::simulate_path_into(
    std::vector<double>& path,
    double T,
    int numSteps,
    std::mt19937& gen,
    std::normal_distribution<double>& dist
) const
{
    double dt = T / numSteps;
    double drift = (riskFreeRate_ - 0.5 * volatility_ * volatility_) * dt;
    double vol   = volatility_ * std::sqrt(dt);

    path[0] = initialPrice_;

    for (int i = 1; i <= numSteps; ++i)
        path[i] = path[i-1] * std::exp(drift + vol * dist(gen));
}
#endif // BLACKSCHOLESMODEL_CPP