#ifndef UTILS_H
#define UTILS_H

static inline int ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

#endif // UTILS_H
