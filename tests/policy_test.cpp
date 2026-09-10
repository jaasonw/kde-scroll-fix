#include "../src/policy.h"

#include <limits>

int main() {
  const QStringList apps{QStringLiteral("chromium")};
  const auto factor = [&](const QString& app, double value) {
    return ScrollFix::factorFor(app, apps, value);
  };
  return !(factor("chromium", 0.25) == 0.25 && factor("firefox", 0.25) == 1.0 &&
           factor("Chromium", 0.25) == 1.0 && factor("", 0.25) == 1.0 &&
           factor("chromium", 1.0) == 1.0 && factor("chromium", 0.05) == 0.05 &&
           factor("chromium", 0.0) == 1.0 && factor("chromium", -1.0) == 1.0 &&
           factor("chromium", 2.0) == 1.0 &&
           factor("chromium", std::numeric_limits<double>::quiet_NaN()) ==
               1.0 &&
           factor("chromium", std::numeric_limits<double>::infinity()) == 1.0 &&
           ScrollFix::validFactor(0.05) && ScrollFix::validFactor(1.0) &&
           !ScrollFix::validFactor(0.049) && !ScrollFix::validFactor(1.01) &&
           !ScrollFix::validFactor(std::numeric_limits<double>::quiet_NaN()));
}
