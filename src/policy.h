#pragma once
#include <QStringList>
#include <cmath>

namespace ScrollFix {
inline bool validFactor(double factor)
{
    return std::isfinite(factor) && factor >= 0.05 && factor <= 1.0;
}

inline double factorFor(const QString &appId, const QStringList &apps, double factor)
{
    return (validFactor(factor) && apps.contains(appId)) ? factor : 1.0;
}
}
