#pragma once
#include <string>

constexpr const char *SOCKET_PATH = "/run/spring_uled.sock";

enum class LEDType { GPIO, PWM, UNKNOWN };

inline LEDType parse_led_type(const std::string &s) {
    if (s == "gpio") return LEDType::GPIO;
    if (s == "pwm")  return LEDType::PWM;
    return LEDType::UNKNOWN;
}

inline const char *to_string(LEDType t) {
    switch (t) {
        case LEDType::GPIO: return "gpio";
        case LEDType::PWM:  return "pwm";
        default:            return "unknown";
    }
}
