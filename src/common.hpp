#pragma once
#include <string>

constexpr const char *SOCKET_PATH = "/run/spring_uled.sock";

enum class LEDType { GPIO, PWM, DUMMY, UNKNOWN };

inline LEDType parse_led_type(const std::string &s) {
    if (s == "gpio") return LEDType::GPIO;
    if (s == "pwm")  return LEDType::PWM;
    if (s == "dummy") return LEDType::DUMMY;
    return LEDType::UNKNOWN;
}

inline const char *to_string(LEDType t) {
    switch (t) {
        case LEDType::GPIO: return "gpio";
        case LEDType::PWM:  return "pwm";
        case LEDType::DUMMY: return "dummy";
        default:            return "unknown";
    }
}
