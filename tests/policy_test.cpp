#include "../src/policy.h"

#include <limits>

namespace {
constexpr double kSlowFactor = 0.25;
constexpr double kTooHighFactor = 2.0;
constexpr double kBelowMinimumFactor = 0.049;
constexpr double kAboveMaximumFactor = 1.01;
}  // namespace

int main() {
  const QStringList apps{QStringLiteral("chromium")};
  const auto factor = [&](const QString& app, double value) {
    return ScrollFix::factorFor(app, apps, value);
  };
  return !(factor("chromium", kSlowFactor) == kSlowFactor &&
           factor("firefox", kSlowFactor) == ScrollFix::kNoChangeFactor &&
           factor("Chromium", kSlowFactor) == ScrollFix::kNoChangeFactor &&
           factor("", kSlowFactor) == ScrollFix::kNoChangeFactor &&
           factor("chromium", ScrollFix::kNoChangeFactor) ==
               ScrollFix::kNoChangeFactor &&
           factor("chromium", ScrollFix::kMinimumFactor) ==
               ScrollFix::kMinimumFactor &&
           factor("chromium", 0.0) == ScrollFix::kNoChangeFactor &&
           factor("chromium", -1.0) == ScrollFix::kNoChangeFactor &&
           factor("chromium", kTooHighFactor) == ScrollFix::kNoChangeFactor &&
           factor("chromium", std::numeric_limits<double>::quiet_NaN()) ==
               ScrollFix::kNoChangeFactor &&
           factor("chromium", std::numeric_limits<double>::infinity()) ==
               ScrollFix::kNoChangeFactor &&
           ScrollFix::validFactor(ScrollFix::kMinimumFactor) &&
           ScrollFix::validFactor(ScrollFix::kNoChangeFactor) &&
           !ScrollFix::validFactor(kBelowMinimumFactor) &&
           !ScrollFix::validFactor(kAboveMaximumFactor) &&
           !ScrollFix::validFactor(std::numeric_limits<double>::quiet_NaN()));
}
