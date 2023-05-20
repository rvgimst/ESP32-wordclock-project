#ifndef WORDCLOCK_IOT_CONFIG_H_
#define WORDCLOCK_IOT_CONFIG_H_

//#include "clock.h"
#include "Display.h"

#include <IotWebConf.h>

// Maximum length of a single IoT configuration value.
#define IOT_CONFIG_VALUE_LENGTH 16

enum NTPState {NTP_Waiting, NTP_Connecting, NTP_Connected};

// Manages clock's configuration portal and propagates settings to the word
// clock.
//
// Some of the configuration parameters are transient, which means that they do
// not retain their values and must be explicitly set every time.
class IotConfig {
  public:
    // Constructs a new IoT configuration with the provided dependencies.
//    IotConfig(WordClock* word_clock);
    IotConfig(Display* display);
    ~IotConfig();

    IotConfig(const IotConfig&) = delete;
    IotConfig& operator=(const IotConfig&) = delete;

    IotConfig(IotConfig&&) = delete;
    IotConfig& operator=(IotConfig&&) = delete;

    // Initializes the IoT configuration portal. Expects dependencies to be
    // initialized.
    void setup();
    // Executes configuration portal's event loop. Expects the object itself to
    // be initialized.
    void loop();

  private:
    // Clears the values of transient parameters.
    void clearTransientParams_();
    // Updates word clock's state to match the current configuration values.
    void updateClockFromParams_();

    // Handles HTTP requests to web server's "/" path.
    void handleHttpToRoot_();
    // Handles HTTP requests to web server's "/config" path.
    void handleHttpToConfig_();
    // Handles configuration changes.
    void handleConfigSaved_();
    // Handles after WiFi connection is established.
    void handleWifiConnected_();
    // To connect to the NTP server and set timezone
    void connectNTP_();
    // Check if NTP connection is established
    bool isNTPConnected_();
    // To update the LED status based on the NTP connection status
    void updateNTPLEDStatus_();

    // Whether IoT configuration was initialized.
    bool initialized_ = false;
    // Whether NTP connection needs to be established.
    bool needNTPConnect_ = false;
    // NTP status
    NTPState NTPState_ = NTP_Waiting;
    // For updating NTP connection regularly
    unsigned long lastNTPConnect_ = 0;

    // Configuration portal's DNS server.
    DNSServer dns_server_;
    // Configuration portal's web server.
    WebServer web_server_;

    // RTC chip interface.
//    RTC_DS3231* rtc_ = nullptr;
    // Word clock state.
//    WordClock* word_clock_ = nullptr;
    Display* display_ = nullptr;

    // Configuration portal's date and time parameter separator.
    IotWebConfSeparator datetime_separator_;

    // Configuration portal's date parameter. Transient.
    //IotWebConfParameter date_param_;
    // Date parameter value.
    //char date_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's time parameter. Transient.
    //IotWebConfParameter time_param_;
    // Time parameter value.
    //char time_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's daylight saving time parameter definition.
    //IotWebConfParameter dst_param_;
    // Daylight saving time parameter value.
    //char dst_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's timezone index from available options.
    IotWebConfParameter timezone_param_;
    // Index of the selected timezone.
    char timezone_value_[IOT_CONFIG_VALUE_LENGTH];
  
    // Configuration portal's appearance parameter separator.
    IotWebConfSeparator display_separator_;

    // Enable AM/PM display.
    IotWebConfParameter show_ampm_param_;
    // Value of the show AMPM parameter.
    char show_ampm_value_[IOT_CONFIG_VALUE_LENGTH];
  
    // Sensitivity parameter for the LDR.
    IotWebConfParameter ldr_sensitivity_param_;
    // Value of the LDR sensitivity parameter.
    char ldr_sensitivity_value_[IOT_CONFIG_VALUE_LENGTH];
  
    // Configuration portal's palette parameter definition.
    // IotWebConfParameter palette_id_param_;
    // Palette parameter value.
    // char palette_id_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's custom palette's color parameter definition.
    IotWebConfParameter color_param_;
    // Custom palette's color parameter value.
    char color_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's period parameter definition.
//   IotWebConfParameter period_param_;
    // Period parameter value.
//   char period_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's debug parameter separator.
    IotWebConfSeparator test_separator_;

    // Configuration portal's clock mode parameter definition.
    IotWebConfParameter clock_mode_param_;
    // Clock mode parameter value.
    char clock_mode_value_[IOT_CONFIG_VALUE_LENGTH];

    // Configuration portal's fast time factor parameter definition.
    // IotWebConfParameter fast_time_factor_param_;
    // Fast time factor parameter value.
    // char fast_time_factor_value_[IOT_CONFIG_VALUE_LENGTH];

    // IotWebConf interface handle.
    IotWebConf iot_web_conf_;
};

#endif  // WORDCLOCK_IOT_CONFIG_H_
