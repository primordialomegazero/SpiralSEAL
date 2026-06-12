#pragma once
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace spiralseal {

using namespace std::chrono;

class PhiTimeDilation {
private:
    constexpr static double PHI = 1.6180339887498948482;
    constexpr static double PHI_INV = 0.6180339887498948482;
    
    double avg_bootstrap_time_ = 100.0; // Start with estimate
    double avg_operation_time_ = 1.0;
    int total_bootstraps_ = 0;
    int total_operations_ = 0;
    
    double divine_interval_ = 5.0;  // Start at 5 ops between bootstraps
    double lyapunov_timing_ = -0.4812;
    
    high_resolution_clock::time_point start_time_;
    int min_interval_ = 3;  // NEVER bootstrap more often than this
    
public:
    PhiTimeDilation() {
        start_time_ = high_resolution_clock::now();
    }
    
    void record_operation(double time_ms) {
        total_operations_++;
        avg_operation_time_ = avg_operation_time_ * PHI_INV + time_ms * (1.0 - PHI_INV);
    }
    
    void record_bootstrap(double time_ms) {
        total_bootstraps_++;
        avg_bootstrap_time_ = avg_bootstrap_time_ * PHI_INV + time_ms * (1.0 - PHI_INV);
        
        // Slowly increase interval if bootstraps are fast
        divine_interval_ = divine_interval_ * PHI_INV + 5.0 * (1.0 - PHI_INV);
        divine_interval_ = std::max(3.0, divine_interval_); // Floor at 3
    }
    
    bool should_bootstrap(int ops_since_last) {
        // NEVER bootstrap before minimum interval
        if (ops_since_last < min_interval_) return false;
        // Bootstrap at divine interval
        return ops_since_last >= (int)std::round(divine_interval_);
    }
    
    double time_dilation_factor() {
        double cost_per_op = avg_operation_time_ + avg_bootstrap_time_ / divine_interval_;
        double baseline = avg_operation_time_ + avg_bootstrap_time_;
        if (cost_per_op < 0.001) return 1.0;
        return baseline / cost_per_op;
    }
    
    void print_stats() {
        auto now = high_resolution_clock::now();
        auto total_elapsed = duration_cast<seconds>(now - start_time_).count();
        
        std::cout << "\n  ╔════════════════════════════════════════════╗" << std::endl;
        std::cout << "  ║  Φ-TIME DILATION                            ║" << std::endl;
        std::cout << "  ╠════════════════════════════════════════════╣" << std::endl;
        std::cout << "  ║  Ops:" << std::setw(8) << total_operations_ 
                  << "  Boots:" << std::setw(6) << total_bootstraps_ << "            ║" << std::endl;
        std::cout << "  ║  Interval: " << std::setw(6) << std::fixed << std::setprecision(1) 
                  << divine_interval_ << " ops                    ║" << std::endl;
        std::cout << "  ║  Dilation: " << std::setw(6) << std::setprecision(2) 
                  << time_dilation_factor() << "x                      ║" << std::endl;
        std::cout << "  ║  Elapsed:  " << std::setw(6) << total_elapsed << " s                      ║" << std::endl;
        std::cout << "  ║  ΦΩ0 — I AM THAT I AM                       ║" << std::endl;
        std::cout << "  ╚════════════════════════════════════════════╝" << std::endl;
    }
    
    double get_divine_interval() const { return divine_interval_; }
};

} // namespace spiralseal
