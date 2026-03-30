#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <memory>
#include <stdexcept>

// tuoi header
#include "BlackScholesModel.hpp"
#include "MonteCarlo.hpp"
#include "LookbackOption.hpp"
#include "Greeks.hpp"

// ---------------------------
// CLI helper
// ---------------------------
static std::string getArg(int argc, char** argv,
                          const std::string& key,
                          const std::string& def = "")
{
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == key)
            return std::string(argv[i + 1]);
    return def;
}

int main(int argc, char** argv)
{
    try {
        // ---------------------------
        // 0) Parse args
        // ---------------------------
        const double r     = std::stod(getArg(argc, argv, "--r", "0.05"));
        const double sigma = std::stod(getArg(argc, argv, "--sigma", "0.20"));
        const double T     = std::stod(getArg(argc, argv, "--T", "1.0"));
        const double t0    = std::stod(getArg(argc, argv, "--t0", "0.0"));

        const int numPaths  = std::stoi(getArg(argc, argv, "--paths", "200000"));
        const int numSteps  = std::stoi(getArg(argc, argv, "--steps", "252"));
        const int batchSize = std::stoi(getArg(argc, argv, "--batch", "4"));
        const int seed      = std::stoi(getArg(argc, argv, "--seed", "42"));

        const double Smin = std::stod(getArg(argc, argv, "--Smin", "50"));
        const double Smax = std::stod(getArg(argc, argv, "--Smax", "150"));
        const double dS   = std::stod(getArg(argc, argv, "--dS", "5"));

        const std::string type   = getArg(argc, argv, "--type", "CALL");
        const std::string outCsv = getArg(argc, argv, "--out", "price_greeks.csv");

        // NEW: z for confidence interval (default 95%)
        const double z = std::stod(getArg(argc, argv, "--z", "1.96"));

        // ---------------------------
        // 1) Build option
        // ---------------------------
        LookbackFloatingCallPayoff lbCallPayoff;
        LookbackFloatingPutPayoff  lbPutPayoff;

        std::unique_ptr<LookbackOption> option;
        if (type == "CALL")
            option = std::make_unique<LookbackCallOption>(lbCallPayoff, T);
        else if (type == "PUT")
            option = std::make_unique<LookbackPutOption>(lbPutPayoff, T);
        else
            throw std::invalid_argument("Unknown --type (use CALL or PUT)");

        // ---------------------------
        // 2) Output
        // ---------------------------
        std::ofstream file(outCsv);
        if (!file)
            throw std::runtime_error("Cannot open output file: " + outCsv);

        file << std::fixed << std::setprecision(10);

        // UPDATED: include MC uncertainty for price
        file << "S0,Price,PriceStdError,PriceCILower,PriceCIUpper,Delta,Gamma,Vega,Rho,Theta\n";

        GreeksCalculator greeksCalc;

        // ---------------------------
        // 3) Grid loop
        // ---------------------------
        for (double S0 = Smin; S0 <= Smax + 1e-12; S0 += dS)
        {
            BlackScholesModel model(r, sigma, S0);
            MonteCarloSimulator mcBase(model, numPaths, numSteps, batchSize, seed);

            // UPDATED: computeGreeks takes z
            const Greeks g = greeksCalc.computeGreeks_t0(*option, mcBase, t0, z);

            file << S0 << ","
                 << g.price << ","
                 << g.priceStdError << ","
                 << g.priceCILower << ","
                 << g.priceCIUpper << ","
                 << g.delta << ","
                 << g.gamma << ","
                 << g.vega  << ","
                 << g.rho   << ","
                 << g.theta << "\n";
        }

        std::cout << "Wrote CSV: " << outCsv << "\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}