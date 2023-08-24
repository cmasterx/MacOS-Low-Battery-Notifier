#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <unordered_map>
#include <stdexcept>
#include <cctype>
#include "debug.hpp"

enum class ChargeState : uint16_t
{
    ERROR = 0,
    DISCHARGING,
    CHARGING,
    NOT_IMPLEMENTED = ERROR
};

struct Battery
{
    uint16_t level;
    ChargeState state;

    operator uint16_t & ()
    {
        return level;
    }
};

constexpr unsigned int MINIMUM_BATTERY_LEVEL = 35;

static const std::unordered_map<std::string, ChargeState> string2enum_map = {
    {"discharging", ChargeState::DISCHARGING},
    {"charging", ChargeState::CHARGING}
};

constexpr bool is_whitespace(char letter)
{
    return letter == ' ' || letter == '\n' || letter == '\r';
}

void displayNotification(std::string title, std::string body)
{
    static constexpr char NOTIFICATION_FORMAT[] = "osascript -e 'display notification \"%s\" with title \"%s\"'";

    const size_t BUFFER_SIZE = sizeof(NOTIFICATION_FORMAT) + title.size() + body.size();
    auto strBuffer = std::make_unique<char []>(BUFFER_SIZE + 1);

    std::snprintf(strBuffer.get(), BUFFER_SIZE, NOTIFICATION_FORMAT, body.c_str(), title.c_str());
    system(strBuffer.get());
}

Battery getBattery()
{
    Battery battery;

    std::array<char, 256> buffer;
    std::string result;
    const char COMMAND[] = "pmset -g batt | awk 'NR==2 {print $3 \" \" $4}'";
    std::unique_ptr<FILE, decltype(&pclose)> proc(popen(COMMAND, "r"), pclose);
    
    if (!proc) return Battery{0, ChargeState::ERROR};

    std::string response;
    while (fread(buffer.data(), 1, buffer.size(), proc.get())) {
        response += buffer.data();
    }

    // --- get battery level ---
    size_t letter_it = 0;
    size_t letter_end = response.size();
    // advance iterator until it finds a non-whitespace character
    while (letter_it < letter_end && is_whitespace(response[letter_it])) {
        ++letter_it;
    }

    if (letter_it >= letter_end) throw std::runtime_error("The command to acquire the battery level either returned an empty string, only whitespaces, or command threw an error.");

    // scan for level
    auto isNumber = [](char letter) {
        return letter >= '0' && letter <= '9';
    };

    unsigned int rawBatteryValue = 0;
    while (letter_it < letter_end && isNumber(response[letter_it])) {
        rawBatteryValue = rawBatteryValue * 10 + (response[letter_it] - '0');
        ++letter_it;
    }

    battery.level = rawBatteryValue;
    
    // --- get battery state ---
    // advance response until a character is found
    while (letter_it < letter_end && !std::isalnum(response[letter_it])) {
        ++letter_it;
    }

    if (letter_it >= letter_end) throw std::runtime_error("The battery's charging state cannot be parsed from the command.");

    // get index of semicolon character
    std::size_t semicolonIdx = letter_it;
    while (semicolonIdx < letter_end && response[semicolonIdx] != ';') ++semicolonIdx;

    std::string rawChargingState = response.substr(letter_it, semicolonIdx - letter_it);
    auto rawChargeType = string2enum_map.find(rawChargingState);
    battery.state = rawChargeType != string2enum_map.end() ? rawChargeType->second : ChargeState::ERROR;

    // debug::println("Starting response");
    // debug::println(response);
    // debug::println("Done!");

    return battery;
}

int main(int argc, char * argv[])
{
    unsigned int minimumBatteryLevel = MINIMUM_BATTERY_LEVEL;
    if (argc > 1) {
        auto newMinLevel = std::stoi(argv[1]);
        if (newMinLevel > 0) minimumBatteryLevel = newMinLevel;
    }
    
    auto battery = getBattery();

//    debug::println("Battery level: ");
//    debug::println(battery.level);

    // std::printf("The batter is %d at level %d", battery.state, battery.level);

    if (battery.state == ChargeState::DISCHARGING && battery.level <= minimumBatteryLevel) {

        std::string msgLog = "Your laptop battery is below " + std::to_string(minimumBatteryLevel) + "%";
        displayNotification("Plug in your charger", msgLog);
    }

   // system("osascript -e 'display notification \"Hello world!\"'");
   return 0;
}
