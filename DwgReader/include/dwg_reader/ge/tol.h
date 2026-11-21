#pragma once

namespace dwr {

/// 几何容差（蓝本 AcGeTol）。
struct Tol {
    double equalPoint = 1.0e-10;
    double equalVector = 1.0e-10;

    static Tol global() { return Tol{}; }
};

}  // namespace dwr
