/*
 * Copyright (c) 2025 Boyd Timothy. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
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
