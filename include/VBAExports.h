#ifndef VBA_EXPORTS_H
#define VBA_EXPORTS_H

// A VBA-friendly C API that wraps the C++ pricer.
//
//
// Notes:
// - Excel/VBA (Windows) expects a .dll. The exports below are written to also
//   compile on Linux/macOS as a shared library, but Excel on Windows cannot load
//   .so/.dylib.
// - Use extern "C" to avoid C++ name mangling.
// - Use __stdcall for VBA friendliness (ignored on Win64 but OK to keep).

#if defined(_WIN32) || defined(_WIN64)
  #define LB_CALLCONV __stdcall
  #if defined(LB_BUILD_DLL)
    #define LB_API extern "C" __declspec(dllexport)
  #else
    #define LB_API extern "C" __declspec(dllimport)
  #endif
#else
  #define LB_CALLCONV
  #if defined(__GNUC__)
    #define LB_API extern "C" __attribute__((visibility("default")))
  #else
    #define LB_API extern "C"
  #endif
#endif

// -----------------------------------------------------------------------------
// Output layout for out10 (10 doubles)
//   [0] price
//   [1] delta
//   [2] gamma
//   [3] vega
//   [4] rho
//   [5] theta
//   [6] priceStdError
//   [7] priceCILower
//   [8] priceCIUpper
//   [9] reserved (0)
// -----------------------------------------------------------------------------

LB_API int LB_CALLCONV Lookback_CallAll(
    double S0, double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* out10);

LB_API int LB_CALLCONV Lookback_PutAll(
    double S0, double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* out10);

// Convenience: compute curves for charts in ONE call (faster than 30 VBA calls)
//
// - spots[i]  = spotStart + i * spotStep
// - prices[i] = Lookback price at spots[i]
// - deltas[i] = Lookback delta at spots[i]
//
// Arrays are assumed contiguous and 0-based (use ReDim arr(0 To n-1) in VBA).

LB_API int LB_CALLCONV Lookback_CallCurve(
    double spotStart, double spotStep, int nPoints,
    double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* spots, double* prices, double* deltas);

LB_API int LB_CALLCONV Lookback_PutCurve(
    double spotStart, double spotStep, int nPoints,
    double r, double sigma, double T,
    int numSteps, int numPaths, int batchSize, int seed,
    double z,
    double* spots, double* prices, double* deltas);

#endif // VBA_EXPORTS_H
