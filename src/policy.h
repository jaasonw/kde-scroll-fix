#ifndef SCROLLFIX_POLICY_H_
#define SCROLLFIX_POLICY_H_
#include <QStringList>
#include <cmath>

namespace ScrollFix {
inline constexpr double kMinimumFactor = 0.05;
inline constexpr double kNoChangeFactor = 1.0;

inline bool validFactor(double factor) {
  return std::isfinite(factor) && factor >= kMinimumFactor &&
         factor <= kNoChangeFactor;
}

inline double factorFor(const QString& appId, const QStringList& apps,
                        double factor) {
  return (validFactor(factor) && apps.contains(appId)) ? factor : 1.0;
}
}  // namespace ScrollFix

#endif  // SCROLLFIX_POLICY_H_
