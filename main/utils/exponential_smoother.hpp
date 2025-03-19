#if !defined(TUNER_EXPONENTIAL_SMOOTHER)
#define TUNER_EXPONENTIAL_SMOOTHER

#include <vector>
#include <algorithm>

class ExponentialSmoother {
public:
    ExponentialSmoother(float alpha) : _alpha(alpha), smoothedValue(0.0), initialized(false) {
        _alpha = std::min(alpha, 1.0f);
        _alpha = std::max(alpha, 0.0f);
    }

    float smooth(float newValue) {
        if (!initialized) {
            smoothedValue = newValue;
            initialized = true; 
        } else {
            smoothedValue = _alpha * newValue + (1 - _alpha) * smoothedValue;
        }
        return smoothedValue;
    }

    void setAlpha(float alpha) {
        _alpha = std::min(alpha, 1.0f);
        _alpha = std::max(alpha, 0.0f);
    }

    void reset() {
        smoothedValue = 0.0;
        initialized = false;
    }

private:
    float _alpha;
    float smoothedValue;
    bool initialized;
};

#endif