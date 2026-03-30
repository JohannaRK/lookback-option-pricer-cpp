#ifndef LOOKBACKOPTION_HPP
#define LOOKBACKOPTION_HPP
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>
#include <omp.h>
class Payoff
{
    public:
        Payoff() = default;
        virtual ~Payoff() = default;          
        // Payoff calculation
        virtual double operator()(double spot, double strike) const = 0; // I want to call Payoff(spot, strike)
};

class VanillaCallPayoff : public Payoff
{
public:
    VanillaCallPayoff() : Payoff() {}
    double operator()(double spot, double strike) const override
    {
        return std::max(spot - strike, 0.0);
    }
};

class VanillaPutPayoff : public Payoff
{
public:
    VanillaPutPayoff() : Payoff() {}
    double operator()(double spot, double strike) const override
    {
        return std::max(strike - spot, 0.0);
    }
};

class LookbackFloatingCallPayoff : public Payoff
{
public:
    LookbackFloatingCallPayoff() : Payoff() {}
    double operator()(double spot, double strike) const override
    {
        return spot - strike; // Always ≥ 0
    }
};

class LookbackFloatingPutPayoff : public Payoff
{
public:
    LookbackFloatingPutPayoff() : Payoff() {}
    double operator()(double spot, double strike) const override
    {
        return strike - spot; // Always ≥ 0 for floating put
    }
};

class VanillaOption
{
    protected:
        const Payoff& payoff_;
        double strike_;
        double maturity_;
    public:
        VanillaOption(const Payoff& payoff, double strike, double maturity)
            : payoff_(payoff), strike_(strike), maturity_(maturity) {}
        double getMaturity() const { return maturity_; }
        void setMaturity(double T) { maturity_ = T; }
        const Payoff& getPayoff() const { return payoff_; };    
        double getStrike() const { return strike_; };
};

class VanillaCallOption : public VanillaOption
{
    public:
        VanillaCallOption(VanillaCallPayoff& payoff, double strike, double maturity)
            : VanillaOption(payoff, strike, maturity) {}
};

class VanillaPutOption : public VanillaOption
{
    public:
        VanillaPutOption(VanillaPutPayoff& payoff, double strike, double maturity)
            : VanillaOption(payoff, strike, maturity) {}
};

class LookbackOption
{
    protected:
        const Payoff& payoff_;
        bool isCall_;
        double maturity_;
    public:
        LookbackOption(const Payoff& payoff, bool isCall, double maturity)
            : payoff_(payoff), isCall_(isCall), maturity_(maturity) {};
        virtual ~LookbackOption() = default;
        double getMaturity() const { return maturity_; }
        void setMaturity(double T) { maturity_ = T; }
        const Payoff& getPayoff() const { return payoff_; };
        bool isCall() const { return isCall_; };
        virtual double computeStrike(const std::vector<double>& path, int timeIndex) const = 0;
        virtual std::unique_ptr<LookbackOption> cloneWithMaturity(double newT) const = 0;
        virtual std::vector<double> computePathStrikes(const std::vector<double>& path) const = 0;
        virtual void computePathStrikesInto(const std::vector<double>& path, std::vector<double>& strikes) const = 0;
};

class LookbackCallOption : public LookbackOption
{
public:
    LookbackCallOption(const LookbackFloatingCallPayoff& payoff, double maturity)
        : LookbackOption(payoff, true, maturity) {}

    // min da timeIndex a T
    double computeStrike(const std::vector<double>& path, int timeIndex) const override
    {
        return *std::min_element(path.begin() + timeIndex, path.end());
    }

    std::unique_ptr<LookbackOption> cloneWithMaturity(double newT) const override
    {
        return std::make_unique<LookbackCallOption>(
            static_cast<const LookbackFloatingCallPayoff&>(payoff_),
            newT
        );
    }

    std::vector<double> computePathStrikes(const std::vector<double>& path) const override
    {
        if (path.empty()) return {};

        const int M = static_cast<int>(path.size()) - 1;
        std::vector<double> strikes(M + 1);

        strikes[M] = path[M];
        for (int i = M - 1; i >= 0; --i)
            strikes[i] = std::min(strikes[i + 1], path[i]);

        return strikes;
    }

    void computePathStrikesInto(const std::vector<double>& path,
                                std::vector<double>& strikes) const override
    {
        if (path.empty()) return;

        const int M = static_cast<int>(path.size()) - 1;

        if (static_cast<int>(strikes.size()) != M + 1)
            strikes.resize(M + 1);

        strikes[M] = path[M];
        for (int i = M - 1; i >= 0; --i)
            strikes[i] = std::min(strikes[i + 1], path[i]);
    }
};


class LookbackPutOption : public LookbackOption
{
public:
    LookbackPutOption(const LookbackFloatingPutPayoff& payoff, double maturity)
        : LookbackOption(payoff, false, maturity) {}

    // max da timeIndex a T
    double computeStrike(const std::vector<double>& path, int timeIndex) const override
    {
        return *std::max_element(path.begin() + timeIndex, path.end());
    }

    std::unique_ptr<LookbackOption> cloneWithMaturity(double newT) const override
    {
        return std::make_unique<LookbackPutOption>(
            static_cast<const LookbackFloatingPutPayoff&>(payoff_),
            newT
        );
    }

    std::vector<double> computePathStrikes(const std::vector<double>& path) const override
    {
        if (path.empty()) return {};

        const int M = static_cast<int>(path.size()) - 1;
        std::vector<double> strikes(M + 1);

        strikes[M] = path[M];
        for (int i = M - 1; i >= 0; --i)
            strikes[i] = std::max(strikes[i + 1], path[i]);

        return strikes;
    }

    void computePathStrikesInto(const std::vector<double>& path,
                                std::vector<double>& strikes) const override
    {
        if (path.empty()) return;

        const int M = static_cast<int>(path.size()) - 1;

        if (static_cast<int>(strikes.size()) != M + 1)
            strikes.resize(M + 1);

        strikes[M] = path[M];
        for (int i = M - 1; i >= 0; --i)
            strikes[i] = std::max(strikes[i + 1], path[i]);
    }
};
#endif // LOOKBACKOPTION_HPP
