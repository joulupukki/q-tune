#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>

#define MIN_MEDIAN_WINDOW_SIZE ((size_t)3)
#define MAX_MEDIAN_WINDOW_SIZE ((size_t)21)

class MedianFilter {
public:
    /// @brief Create a new smoother with the specified window size.
    explicit MedianFilter(size_t windowSize, bool useMovingMode) : useMovingMode(useMovingMode), calculatedValue(-1.0f) {
        setWindowSize(windowSize);
    }

    /// @brief Sets the window size after the class has been constructed.
    /// @param size The new window size.
    void setWindowSize(size_t size) {
        this->windowSize = size;
        if (this->windowSize % 2 == 0) { // Ensure the window size is odd
            this->windowSize += 1;
        }
        this->windowSize = std::min(this->windowSize, MAX_MEDIAN_WINDOW_SIZE);
        this->windowSize = std::max(this->windowSize, MIN_MEDIAN_WINDOW_SIZE);
        this->middleIndex = this->windowSize / 2;
    }

    /// @brief Add a value to the smoother.
    /// @param value The new value.
    /// @return Returns the calculated value IF it's available or -1 if not available.
    float addValue(float value) {
        // Always keep the values sorted
        auto it = std::upper_bound(values.begin(), values.end(), value);
        values.insert(it, value);
        size_t size = values.size();
        if (useMovingMode) {
            if (size > windowSize) {
                values.erase(values.begin());
                size = windowSize;
            }
            if (size == windowSize) {
                // Return the middle value
                calculatedValue = values[middleIndex];

                return calculatedValue;
            } else {
                // Return the "middle" value of the current set
                int tempMiddleIndex = size / 2;
                calculatedValue = values[tempMiddleIndex];
                return calculatedValue;
            }
        } else if (size == windowSize) {
            // Return the middle value
            calculatedValue = values[middleIndex];

            // Clear all the values
            values.clear();

            return calculatedValue;
        }

        return -1; // Not enough values yet
    }

    /// @brief Resets the average and prep for reuse.
    void reset() {
        values.clear();
        calculatedValue = -1.0f;
    }

private:
    size_t windowSize;
    bool useMovingMode;
    int middleIndex;
    std::vector<float> values;
    float calculatedValue;
};
