#ifndef MONTECARLO_HPP
#define MONTECARLO_HPP
#include "BlackScholesModel.hpp"
#include "LookbackOption.hpp"
#include <vector>
#include <stdexcept>
class MonteCarloSimulator
{
    private:
        const BlackScholesModel& model_;
        int numPaths_;
        int numSteps_;
        int batchSize_;
        const unsigned int seed;
    public:
        MonteCarloSimulator(const BlackScholesModel& model, int numPaths, int numSteps, int batchSize = 1, unsigned int seed = 42) : 
                            model_(model), numPaths_(numPaths), numSteps_(numSteps), batchSize_(batchSize), seed(seed) {};
        MonteCarloSimulator& setBatchSize(int batchSize) 
        { 
            if (batchSize <= 0)
                throw std::invalid_argument("batchSize must be > 0");
            batchSize_ = batchSize; 
            return *this; 
        };
        const BlackScholesModel& getModel() const { return model_; };
        int getNumPaths() const { return numPaths_; };
        int getNumSteps() const { return numSteps_; };
        int getBatchSize() const { return batchSize_; };
        unsigned int getSeed() const { return seed; };
        struct MCResult
        {
            double price;
            double stdError;
            double ciLower;
            double ciUpper;
        };
        MCResult simulateOptionPriceControlVariate_t0(const LookbackOption& option, double t0, double z = 1.96) const;
};
#endif // MONTECARLO_HPP
