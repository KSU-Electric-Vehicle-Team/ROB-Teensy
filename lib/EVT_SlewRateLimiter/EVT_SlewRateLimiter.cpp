#include "EVT_SlewRateLimiter.hpp"

namespace MotorControls {
  SlewRateLimiter::SlewRateLimiter(float maxChange, float initialVal) {
    maxIncrease = maxDecrease = maxChange;

    lastValue = initialVal;

    currentTime = pdTICKS_TO_MS(xTaskGetTickCount());
    lastTime = pdTICKS_TO_MS(xTaskGetTickCount());
  }


  SlewRateLimiter::SlewRateLimiter(float maxPosChange, float maxNegChange, float initialVal) {
    maxIncrease = maxPosChange;
    maxDecrease = maxNegChange;

    lastValue = initialVal;

    currentTime = pdTICKS_TO_MS(xTaskGetTickCount());
    lastTime = pdTICKS_TO_MS(xTaskGetTickCount());
  }


  float SlewRateLimiter::calculate(float targetValue) {
    currentTime = pdTICKS_TO_MS(xTaskGetTickCount()); // Get the current time
    timeChange = currentTime - lastTime;              // Measure the change in time
    deltaValue = targetValue - lastValue;

    if (deltaValue > 0) {
      maxDelta = maxIncrease * (timeChange / 1'000);
      deltaValue = min(deltaValue, maxDelta);
    } else if (deltaValue < 0) {
      maxDelta = maxDecrease * (timeChange / 1'000);
      deltaValue = max(deltaValue, -maxDelta);
    }

    lastValue += deltaValue;
    lastTime = pdTICKS_TO_MS(xTaskGetTickCount()); // Record the time at the end of the current iteration

    return lastValue;
  }


  void SlewRateLimiter::setRate(float rate) {
    maxIncrease = maxDecrease = rate;
  }


  void SlewRateLimiter::setRate(float pos, float neg) {
    maxIncrease = pos;
    maxDecrease = neg;
  }


  void SlewRateLimiter::setLastValue(float val) {
    lastValue = val;
  }
}