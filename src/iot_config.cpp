#include "iot_config.h"
//#include "clock.h"
#include "Display.h"
#include "Timezones.h"
#include "Clockmodes.h"

#include <IotWebConf.h>
#include <WiFi.h>
#include <RTClib.h> // RVG: only for data structures

// Name of this IoT object.
#define THING_NAME "WordClockLTX"
// Initial WiFi access point password.
#define INITIAL_WIFI_AP_PASSWORD "12345678"
// IoT configuration version. Change this whenever IotWebConf object's
// configuration structure changes.
#define CONFIG_VERSION "v1"
// Default timezone index from Timezones.h (Paris).
#define DEFAULT_TIMEZONE "351" // 351=Amsterdam 385=Paris 153=New York
// Port used by the IotWebConf HTTP server.
#define WEB_SERVER_PORT 80
// -- Status indicator pin.
//    Rapid blinking - Entered Access Point mode. 
//      This means the device creates an own WiFi network around it. You can connect to the 
//      device with your smartphone (or WiFi capable computer).
//    Alternating on/off blinking - Trying to connect the configured WiFi network.
//    Mostly on with occasional short off-flash - The device is online..
#define IOT_WEB_STATUS_PIN 2
// -- set up a push button. If push button is detected
//    to be pressed at boot time, the Thing will use the initial
//    password for accepting connections in Access Point mode. This
//    is usefull e.g. when custom password was lost.
#define IOT_AP_CONFIG_PIN 4
// -- setup an indicator LED for the NTP time state
//    Off             - Waiting, no NTP activity
//    Blinking        - Connecting to NTP server to sync time
//    On continuously - Connected to NTP server, using NTP time for the clock
#define NTP_STATUS_PIN 16
#define NTP_BLINK_MS 300
#define NTP_POLL_SHORT_DELAY_SECONDS 20 // Once every 20s
#define NTP_POLL_LONG_DELAY_SECONDS 3*3600 // Once every 3 hours

// HTTP OK status code.
#define HTTP_OK 200

// HTTP MIME type.
#define MIME_HTTP "text/html"

// NTP CLOCK ========================================================================
// NTP server. Maintains local time plus timezone settings
#define NTP_SVR   "pool.ntp.org"
// NTP servers
//#define NTP_SVR   "pool.ntp.org"
//#define NTP_SVR   "europe.pool.ntp.org"
//#define NTP_SVR   "north-america.pool.ntp.org"

// Time zone settings
//#define CLK_TZ     "CET-1CEST,M3.5.0,M10.5.0/3" // Amsterdam
#define CLK_TZ     "EST5EDT,M3.2.0,M11.1.0" // America/New-York
//#define NTP_LT_TIMEOUT 3000 // [ms] waiting time for time from NTP server

namespace {
  // An SVG logo for the Word Clock
  const char LOGO_DATA_URI[] PROGMEM = "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 480 480'%3E%3Cpath d='M0 0h480v480H0z'/%3E%3Cg fill='%23fff'%3E%3Cpath d='M150.692 163.411h-6.563l-11.522-38.242q-.82-2.54-1.836-6.406-1.016-3.867-1.055-4.649-.86 5.157-2.735 11.29L115.81 163.41h-6.563l-15.195-57.109h7.031l9.024 35.273q1.875 7.422 2.734 13.438 1.055-7.148 3.125-13.984l10.234-34.727h7.031l10.742 35.04q1.875 6.054 3.164 13.671.742-5.547 2.813-13.516l8.984-35.195h7.031z' aria-label='W'/%3E%3Cpath d='M268.677 134.718q0 13.711-6.954 21.562-6.914 7.852-19.258 7.852-12.617 0-19.492-7.695-6.835-7.735-6.835-21.797 0-13.945 6.875-21.602 6.875-7.696 19.53-7.696 12.306 0 19.22 7.813t6.914 21.562zm-45.508 0q0 11.602 4.921 17.617 4.961 5.977 14.375 5.977 9.493 0 14.336-5.977t4.844-17.617q0-11.523-4.844-17.46-4.804-5.977-14.258-5.977-9.492 0-14.453 6.015-4.922 5.977-4.922 17.422z' aria-label='O'/%3E%3Cpath d='M154.135 244.514q0 14.14-7.696 21.64-7.656 7.462-22.07 7.462h-15.82v-57.11h17.5q13.32 0 20.703 7.383t7.383 20.625zm-7.032.234q0-11.172-5.625-16.836-5.586-5.664-16.64-5.664h-9.65v45.625h8.087q11.875 0 17.852-5.82 5.976-5.86 5.976-17.305z' aria-label='D'/%3E%3Cpath d='M339.34 139.658v23.75h-6.641v-57.109h15.664q10.508 0 15.508 4.023 5.039 4.024 5.039 12.11 0 11.327-11.484 15.311l15.508 25.664h-7.852l-13.828-23.75zm0-5.703h9.101q7.031 0 10.312-2.773 3.281-2.813 3.281-8.399 0-5.664-3.36-8.164-3.32-2.5-10.702-2.5h-8.633z' aria-label='R'/%3E%3Cpath d='M245.577 219.873q-9.414 0-14.883 6.289-5.43 6.25-5.43 17.148 0 11.211 5.235 17.344 5.273 6.094 15 6.094 5.976 0 13.633-2.149v5.82q-5.938 2.227-14.648 2.227-12.617 0-19.492-7.656-6.836-7.656-6.836-21.758 0-8.828 3.281-15.469 3.32-6.64 9.531-10.234 6.25-3.594 14.688-3.594 8.985 0 15.703 3.282l-2.812 5.703q-6.485-3.047-12.97-3.047z' aria-label='C'/%3E%3Cpath d='M156.44 354.9q0 13.711-6.953 21.562-6.914 7.852-19.258 7.852-12.617 0-19.492-7.696-6.836-7.734-6.836-21.797 0-13.945 6.875-21.602 6.875-7.695 19.531-7.695 12.305 0 19.219 7.813t6.914 21.562zm-45.508 0q0 11.602 4.922 17.617 4.96 5.977 14.375 5.977 9.492 0 14.336-5.977t4.844-17.617q0-11.523-4.844-17.461-4.805-5.977-14.258-5.977-9.492 0-14.453 6.016-4.922 5.977-4.922 17.422z' aria-label='O'/%3E%3Cpath d='M336.73 273.613v-57.109h6.641v51.094h25.195v6.016z' aria-label='L'/%3E%3Cpath d='M373.956 383.337h-7.813l-20.82-27.695-5.976 5.312v22.383h-6.64v-57.109h6.64v28.32l25.898-28.32h7.852l-22.97 24.805z' aria-label='K'/%3E%3Cpath d='M245.577 329.87q-9.414 0-14.883 6.29-5.43 6.25-5.43 17.147 0 11.211 5.235 17.344 5.273 6.094 15 6.094 5.976 0 13.633-2.148v5.82q-5.938 2.226-14.648 2.226-12.617 0-19.492-7.656-6.836-7.656-6.836-21.758 0-8.828 3.281-15.469 3.32-6.64 9.531-10.234 6.25-3.594 14.688-3.594 8.985 0 15.703 3.282l-2.812 5.703q-6.485-3.047-12.97-3.047z' aria-label='C'/%3E%3C/g%3E%3Cg fill='%23fff'%3E%3Ccircle cx='37.785' cy='37.968' r='7.906'/%3E%3Ccircle cx='439.98' cy='38.047' r='7.906'/%3E%3Ccircle cx='37.785' cy='440.74' r='7.906'/%3E%3Ccircle cx='439.98' cy='440.82' r='7.906'/%3E%3C/g%3E%3C/svg%3E";
  
  // Custom meta tags to be inserted in the head.
  const char CUSTOM_HTML_META_START[] = "<meta name=\"theme-color\" content=\"#121212\" />\n<link rel=\"icon\" href=\"";
  const char CUSTOM_HTML_META_END[] = "\" type=\"image/svg+xml\" />\n";
  
  // Custom Javascript block that will be added to the header.
  // See customconfig.js for human-readable version.
  const char CUSTOMHTML_SCRIPT_INNER[] PROGMEM = "document.addEventListener(\"DOMContentLoaded\",function(d){if(d=document.querySelector(\"label[for=iwcThingName]\"))d.innerText=\"Clock name\";if(d=document.querySelector(\"label[for=iwcApPassword]\"))d.innerText=\"AP password (login: admin)\";document.querySelectorAll(\"input[type=password]\").forEach(function(a){var b=document.createElement(\"input\");b.classList.add(\"pwtoggle\");b.type=\"button\";b.value=\"\\ud83d\\udc41\\ufe0f\";a.insertAdjacentElement(\"afterend\",b);b.onclick=function(){\"password\"===a.type?(a.type=\
  \"text\",b.value=\"\\ud83d\\udd12\"):(a.type=\"password\",b.value=\"\\ud83d\\udc41\\ufe0f\")}});(d=document.querySelector(\"form\"))&&d.addEventListener(\"submit\",function(){var a=document.querySelector(\"button[type=submit]\");a.innerText=\"Saving...\";a.toggleAttribute(\"disabled\",!0)});document.querySelectorAll(\"input[data-options]\").forEach(function(a){var b=a.value,h=a.getAttribute(\"data-options\").split(\"|\"),f=document.createElement(\"select\");f.name=a.name;f.id=a.id;\"\"===b&&f.appendChild(document.createElement(\"option\"));\
  var c=null;h.forEach(function(m,n){var e=m.split(\"/\"),g=m;1<e.length?(g=e.splice(0,1)[0],c&&g==c.label||(c&&f.appendChild(c),c=document.createElement(\"optgroup\"),c.label=g),g=e.join(\" / \")):c&&(f.appendChild(c),c=null);e=document.createElement(\"option\");e.value=n;e.innerText=g;n==b&&e.toggleAttribute(\"selected\");c?c.appendChild(e):f.appendChild(e)});c&&f.appendChild(c);a.id+=\"-d\";a.insertAdjacentElement(\"beforebegin\",f);a.parentElement.removeChild(a)});document.querySelectorAll(\"input[type=range]\").forEach(function(a){var b=\
  a.getAttribute(\"data-labels\"),h=b&&b.split(\"|\");b=function(){a.setAttribute(\"data-label\",h?h[parseInt(a.value,10)]||a.value:a.value)};a.oninput=b;b()});var k=document.getElementById(\"ntp_enabled\");if(k){var p=function(a){document.querySelectorAll(\"#date, #time\").forEach(function(b){b.parentElement.style.display=a?\"none\":\"\"});document.getElementById(\"timezone\").parentElement.style.display=a?\"\":\"none\"};k.addEventListener(\"change\",function(a){p(1==k.value)});p(1==k.value)}var l=document.querySelector(\"input[type=color]\");\
  l&&(d=function(){document.querySelector(\".logoContainer\").style.backgroundColor=l.value},l.addEventListener(\"input\",d),d())});";
  
  // Custom style added to the style tag.
  const char CUSTOMHTML_STYLE_INNER[] PROGMEM = "\n\
  body {\
    color: #eee;\
    margin: 0;\
  background: #121212;\
    font: 100% system-ui;}\
  a {\
    color: #16a1e7;}\
  div {\
    padding: 0;}\
  .logo {\
    width: 80vw;\
    max-width: 160px;\
    display: block;\
    mix-blend-mode: multiply;}\
  .logoContainer {\
    background: white;\
    transition: background 3s;\
    display: inline-block;\
    margin: 40px auto;\
    padding: 0;}\
  .logoContainer, fieldset {\
    box-shadow: 0px 0px 15px 1px #ffffff7d;}\
  .pwtoggle {\
    padding: 0 5px;\
    position: absolute;\
    right: 3px;\
    top: 40px;\
    border: none;\
    background: transparent;\
    height: 34px;\
    outline: none;}\
  fieldset {\
    padding: 40px 15px 20px;\
    margin-bottom: 40px;\
    border: none;\
    background: black;\
    border-radius: 0;\
    position: relative;\
    width: calc(100vw - 100px);\
    max-width: 440px;}\
  fieldset > div {\
    position: relative;\
    margin: 0;\
    padding: 0;\
    display: flex;\
    flex-direction: column;}\
  input, select {\
    margin: 0;\
    padding: 5px;\
    width: auto;\
    line-height: 20px;}\
  input[type=\"range\"] {\
    margin-left: 30px;\
    position: relative;\
    padding: 0;}\
  input[type=\"range\"]:before {\
    content: attr(data-label);\
    color: #eee;\
    position: absolute;\
    left: -30px;\
    display: inline;\
    width: 25px;\
    line-height: 20px;}\
  label {margin: 15px 0 5px;}\
  legend, fieldset:first-child:after {\
    font-weight: lighter;\
    padding: 0;\
    font-size: 1.2em;\
    text-align: center;\
    position: absolute;\
    top: 15px;\
    display: block;\
    left: 0;\
    right: 0;\
    text-transform: uppercase;}\
  fieldset:first-child:after {content: \"Network\";}\
  button {\
    border-radius: 0;\
    text-transform: uppercase;}\
  form + div {padding: 20px 0 15px 0;}\
  body > div > div:last-child {\
    margin-top: -20px;\
    float: right;}\
  }\n";
  
  // Custom HTML element will be added at the beginning of the body element.
  const char CUSTOMHTML_BODY_INNER_START[] PROGMEM = "<header><div class=\"logoContainer\"><img class=\"logo\" src=\"";
  const char CUSTOMHTML_BODY_INNER_END[] PROGMEM = "\"/></div></header>\n";
  
  class CustomHtmlFormatProvider : public IotWebConfHtmlFormatProvider
  {
    protected:
      String getHead() override
      {
        String head = IotWebConfHtmlFormatProvider::getHead();
        head.replace("{v}", THING_NAME);
        return head + String(FPSTR(CUSTOM_HTML_META_START)) +
               String(FPSTR(LOGO_DATA_URI)) +
               String(FPSTR(CUSTOM_HTML_META_END));
      }
      String getScriptInner() override
      {
        return IotWebConfHtmlFormatProvider::getScriptInner() +
               String(FPSTR(CUSTOMHTML_SCRIPT_INNER));
      }
      String getStyleInner() override
      {
        return IotWebConfHtmlFormatProvider::getStyleInner() +
               String(FPSTR(CUSTOMHTML_STYLE_INNER));
      }
      String getBodyInner() override
      {
        return String(FPSTR(CUSTOMHTML_BODY_INNER_START)) +
               String(FPSTR(LOGO_DATA_URI)) +
               String(FPSTR(CUSTOMHTML_BODY_INNER_END)) +
               IotWebConfHtmlFormatProvider::getBodyInner();
      }
  };
  // An instance must be created from the class defined above.
  CustomHtmlFormatProvider customHtmlFormatProvider;
  
  // Attempts to parse `str` as a date in yyyy-mm-dd format. If it succeeds,
  // returns true and populates `year`, `month` and `day` with the corresponding
  // date values. If it fails, returns false and leaves other parameters unchanged.
  bool parseDateValue(const char* str, uint16_t* year, uint8_t* month,
                      uint8_t* day) {
    unsigned int parsed_year;
    unsigned int parsed_month;
    unsigned int parsed_day;
  
    int result = sscanf(str, "%u-%u-%u", &parsed_year, &parsed_month,
                        &parsed_day);
    if (result != 3 || parsed_year < 2000 || parsed_year > 9999 ||
        parsed_month == 0 || parsed_month > 12 || parsed_day == 0 ||
        parsed_day > 31) {
      Serial.print("[INFO] Could not parse date value \"");
      Serial.print(str);
      Serial.println("\".");
      return false;
    }
  
    *year = parsed_year;
    *month = parsed_month;
    *day = parsed_day;
    return true;
  }
  
  // Attempts to parse `str` as a time in hh:mm:ss format. If it succeeds, returns
  // true and populates `hour`, `minute` and `second` with the corresponding time
  // values. If it fails, returns false and leaves other parameters unchanged.
  bool parseTimeValue(const char* str, uint8_t* hour, uint8_t* minute,
                      uint8_t* second) {
    unsigned int parsed_hour;
    unsigned int parsed_minute;
    unsigned int parsed_second;
  
    int result = sscanf(str, "%u:%u:%u", &parsed_hour, &parsed_minute,
                        &parsed_second);
    if (result != 3 || parsed_hour > 23 || parsed_minute > 59 ||
        parsed_second > 59) {
      Serial.print("[INFO] Could not parse time value \"");
      Serial.print(str);
      Serial.println("\".");
      return false;
    }
  
    *hour = parsed_hour;
    *minute = parsed_minute;
    *second = parsed_second;
    return true;
  }
  
  //// Attempts to parse the provided `date_str` and `time_str` as date and time,
  //// respectively. If set values are successfully parsed, adjusts `rtc` to the new
  //// date and time.
  //void parseAndSetDateTime(WordClock* word_clock, const char* date_str,
  //                         const char* time_str) {
  //  const DateTime now = word_clock->getCurrentTime();
  //  uint16_t year = now.year();
  //  uint8_t month = now.month();
  //  uint8_t day = now.day();
  //  uint8_t hour = now.hour();
  //  uint8_t minute = now.minute();
  //  uint8_t second = now.second();
  //  bool datetime_changed = false;
  //
  //  if (date_str[0] != 0 && parseDateValue(date_str, &year, &month, &day)) {
  //    datetime_changed = true;
  //  }
  //  if (time_str[0] != 0 && parseTimeValue(time_str, &hour, &minute, &second)) {
  //    datetime_changed = true;
  //  }
  //
  //  if (datetime_changed) {
  //    //rtc->adjust(DateTime(year, month, day, hour, minute, second));
  //  }
  //}
  
  // Attempts to parse `str` as a color in "#RRGGBB" format. If it succeeds,
  // returns the parsed value. If it fails, returns `default_value`.
  RgbColor parseColorValue(const char* str, const RgbColor& default_value) {
    bool has_error = str[0] != '#' || str[7] != 0;
    for (int i = 1; i <= 6; i++) {
      has_error &= !isxdigit(str[i]);
    }
    if (has_error) {
      Serial.print("[INFO] Could not parse color value \"");
      Serial.print(str);
      Serial.println("\".");
      return default_value;
    }
  
    const int parsed_value = strtol(str + 1, nullptr, 16);
    const uint8_t red = (parsed_value >> 16) & 0xFF;
    const uint8_t green = (parsed_value >> 8) & 0xFF;
    const uint8_t blue = parsed_value & 0xFF;
    return RgbColor(red, green, blue);
  }
  
  // Attempts to parse `str` as a number in the interval `[min_value, max_value].
  // If it succeeds, returns the parsed value. If it fails, returns
  // `default_value`.
  int parseNumberValue(const char* str, int min_value, int max_value,
                       int default_value) {
    char* end_ptr = nullptr;
    int parsed_value = strtol(str, &end_ptr, 10);
    if (*end_ptr != 0 || parsed_value < min_value || parsed_value > max_value) {
      Serial.print("[INFO] Could not parse number value \"");
      Serial.print(str);
      Serial.println("\".");
      return default_value;
    }
    return parsed_value;
  }
  
//  // Attempts to parse `str` as a number 0 or 1 and casts it as a boolean. If it
//  // fails, returns false.
//  bool parseBooleanValue(const char *str)
//  {
//    return static_cast<bool>(parseNumberValue(str, 0, 1, 0));
//  }

}  // namespace


  // From IotWebConf.h:
  // /**
  //  * Create a parameter for the config portal.
  //  *
  //  *   @label - Displayable label at the config portal.
  //  *   @id - Identifier used for HTTP queries and as configuration key. Must not contain spaces nor other special characters.
  //  *   @valueBuffer - Configuration value will be loaded to this buffer from the EEPROM.
  //  *   @length - The buffer should have a length provided here.
  //  *   @type (optional, default="text") - The type of the html input field.
  //  *       The type="password" has a special handling, as the value will be overwritten in the EEPROM
  //  *       only if value was provided on the config portal. Because of this logic, "password" type field with
  //  *       length more then IOTWEBCONF_PASSWORD_LEN characters are not supported.
  //  *   @placeholder (optional) - Text appear in an empty input box.
  //  *   @defaultValue (optional) - Value should be pre-filled if none was specified before.
  //  *   @customHtml (optional) - The text of this parameter will be added into the HTML INPUT field.
  //  */
  // IotWebConfParameter(
  //   const char* label, const char* id, char* valueBuffer, int length,
  //   const char* type = "text", const char* placeholder = NULL,
  //   const char* defaultValue = NULL, const char* customHtml = NULL,
  //   boolean visible = true);

  // /**
  //  * Same as normal constructor, but config portal does not render a default
  //  * input field for the item, instead uses the customHtml provided. Note the
  //  * @type parameter description above!
  //  */
  // IotWebConfParameter(
  //     const char* id, char* valueBuffer, int length, const char* customHtml,
  //     const char* type = "text");

IotConfig::IotConfig(Display* display)
  : web_server_(WEB_SERVER_PORT), display_(display),
    datetime_separator_("Date and time"),
    // date_param_("Date", "date", date_value_, IOT_CONFIG_VALUE_LENGTH, "date",
    //             "yyyy-mm-dd", nullptr, "pattern='\\d{4}-\\d{1,2}-\\d{1,2}'"),
    // time_param_("Time", "time", time_value_, IOT_CONFIG_VALUE_LENGTH, "time",
    //             "hh:mm:ss", nullptr,
    //             "pattern='\\d{1,2}:\\d{1,2}:\\d{1,2}' step='1'"),
    // dst_param_("Daylight saving time? (0=false, 1=true)", "dst", dst_value_,
    //            IOT_CONFIG_VALUE_LENGTH, "number", "0", "0",
    //            "pattern='[01]' min='0' max='1' "
    //            "style='max-width: 2em; display: block;'"),
    timezone_param_("Time zone", "timezone", timezone_value_, IOT_CONFIG_VALUE_LENGTH,
                    "number", DEFAULT_TIMEZONE, DEFAULT_TIMEZONE, locationOptions),
    display_separator_("Display"),
    show_ampm_param_(
       "AM/PM indicator", "show_ampm", show_ampm_value_,
       IOT_CONFIG_VALUE_LENGTH, "range", "0", "0",
       "pattern='[01]' min='0' max='1' "
       "style='width: 40px;' data-labels='Off|On' min='0' max='1' step='1'"),
    ldr_sensitivity_param_(
       "Light sensor sensitivity", "ldr_sensitivity", ldr_sensitivity_value_,
       IOT_CONFIG_VALUE_LENGTH, "range", "5", "5",
       "min='0' max='10' step='1' data-labels='Off'"),    
    // palette_id_param_(
    //   "Color palette number (0=custom)", "palette_id", palette_id_value_,
    //   IOT_CONFIG_VALUE_LENGTH, "number", "1", "1",
    //   "pattern='\\d+' min='0' max='7' "
    //   "style='max-width: 2em; display: block;'"),
    color_param_("Custom color", "color", color_value_,
                   IOT_CONFIG_VALUE_LENGTH, "color", "#RRGGBB", "#EFEBD8",
                   "pattern='#[0-9a-fA-F]{6}' "
                   "style='border-width: 1px; padding: 1px;'"),
    // period_param_("Show period? (0=false, 1=true)", "period", period_value_,
    //               IOT_CONFIG_VALUE_LENGTH, "number", "0", "0",
    //               "pattern='[01]' min='0' max='1' "
    //               "style='max-width: 2em; display: block;'"),
    test_separator_("Test"),
    clock_mode_param_(
      "Clock mode", "clock_mode", clock_mode_value_,
      IOT_CONFIG_VALUE_LENGTH, "number", "0", "0", clockModeOptions),
      // "pattern='\\d+' min='0' max='6' "
      // "style='max-width: 2em; display: block;'"),
    // fast_time_factor_param_(
    //   "Fast time factor", "fast_time_factor", fast_time_factor_value_,
    //   IOT_CONFIG_VALUE_LENGTH, "number", "30", "30",
    //   "pattern='\\d+' min='1' max='3600' "
    //   "style='max-width: 4em; display: block;'"),
    find_word_param_(
      "Find word", "find_word", find_word_value_,
      IOT_CONFIG_VALUE_LENGTH, "text", "", ""),
    iot_web_conf_(THING_NAME, &dns_server_, &web_server_,
                  INITIAL_WIFI_AP_PASSWORD, CONFIG_VERSION)
{
  this->show_ampm_value_[0] = '\0';
  this->ldr_sensitivity_value_[0] = '\0';
}

IotConfig::~IotConfig() {}

void IotConfig::clearTransientParams_() {
  // date_value_[0] = 0;
  // time_value_[0] = 0;
}

// Note that IotWebConf does not currently work with parameters that require an
// HTML checkbox input to set in the configuration portal, so we have to use the
// workaround of representing booleans as 0 or 1 integers.
void IotConfig::updateClockFromParams_() {
  Serial.println("=IotConfig::updateClockFromParams_()");
  //parseAndSetDateTime(word_clock_, date_value_, time_value_);

  display_->setClockMode(static_cast<ClockMode>(
                            parseNumberValue(clock_mode_value_, 0,
                                  static_cast<int>(ClockMode::MAX_VALUE),
                                  static_cast<int>(ClockMode::REAL_TIME))));
//  word_clock_->setDst(static_cast<bool>(
//                        parseNumberValue(dst_value_, 0, 1, 0)));

  // Timezone
  // Timezone is handled within the iot_config class, so we call the local NTP
  // member function rather than calling the word_clock object
  //    word_clock_->setTimezone(
  //            parseNumberValue(timezone_value_, DEFAULT_TIMEZONE, 0, 459));
  connectNTP_();

//  word_clock_->setFastTimeFactor(
//    parseNumberValue(fast_time_factor_value_, 1, 3600, 30));
//  word_clock_->setPaletteId(
//    parseNumberValue(palette_id_value_, 0, PALETTE_COUNT, 1));
//  word_clock_->setPeriod(static_cast<bool>(
//                           parseNumberValue(period_value_, 0, 1, 0)));
//
//   RgbColor(190, 9, 0)));
//   RgbColor(203, 91, 10)));
//   RgbColor(254, 204, 92)));
  display_->setColor(parseColorValue(color_value_, RgbColor(239, 235, 216)));
  display_->setShowAmPm(static_cast<bool>(
                        parseNumberValue(show_ampm_value_, 0, 1, 0)));
  display_->setSensorSensitivity(parseNumberValue(ldr_sensitivity_value_, 0, 10, 5)); 
  display_->setFindWord(find_word_value_, IOT_CONFIG_VALUE_LENGTH); 
}

void IotConfig::handleHttpToRoot_() {
  static const char html[] =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta name='viewport' content='width=device-width, "
    "initial-scale=1, user-scalable=no'/>"
    "<title>Word clock LT</title>"
    "</head>"
    "<body>"
    "<h1>Word Clock LT</h1>"
    "<ul><li><a href='config'>Settings</a></li></ul>"
    "</body>"
    "</html>\n";

  if (iot_web_conf_.handleCaptivePortal()) return;
  web_server_.send(HTTP_OK, MIME_HTTP, html);
}

void IotConfig::handleHttpToConfig_() {
  clearTransientParams_();
  iot_web_conf_.handleConfig();
}

void IotConfig::handleConfigSaved_() {
  updateClockFromParams_();
}

void IotConfig::handleWifiConnected_() {
  Serial.println("WiFi connected. Initiating NTP proces...");
  NTPState_ = NTP_Connecting;
  connectNTP_();
}

// Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
// Change with sntp_set_update_delay(ms)
//  RVG changed this since Arduino time implementation doesn't seem to support the function prototype below
//  configTime(CLK_TZ, CLK_SVR1, CLK_SVR2, CLK_SVR3);
void IotConfig::connectNTP_() {
  struct tm timeinfo;
  bool gotLocalTime = false;
  //  String timezone(CLK_TZ);

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi not connected, cannot set time from NTP server.");
    NTPState_ = NTP_Waiting;
    return;
  }

  Serial.println((String)"connectNTP_: Connecting to NTP server: " + NTP_SVR);
  configTime(0, 0, NTP_SVR);    // First connect to NTP server, with 0 TZ offset

  // Timezone
  int tz = parseNumberValue(timezone_value_, 0, 459, 0);
  Serial.printf(" Setting Timezone to %s (%d)\n",  posix[tz], tz);
  configTzTime(posix[tz], NTP_SVR);
  //  Serial.printf(" Setting Timezone to %s\n",  timezone.c_str());
  //  setenv("TZ", timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  //  tzset();

  //  Serial.print(" NTP: Obtaining local time");
  //  gotLocalTime = getLocalTime(&timeinfo, 10);
  //  Serial.print(&timeinfo, " %A, %B %d %Y %H:%M:%S");
  //  Serial.printf(" (%d)\n", gotLocalTime);

  if (isNTPConnected_()) {
    NTPState_ = NTP_Connected;
  } else {
    NTPState_ = NTP_Connecting;
  }
  lastNTPConnect_ = millis();
}

bool IotConfig::isNTPConnected_() {
  struct tm timeinfo;
  bool getLTStatus = getLocalTime(&timeinfo, 10);
  return getLTStatus;
}

void IotConfig::updateNTPLEDStatus_() {
  static bool ntpPinHigh = false;
  static unsigned long lastBlinkTime = 0;
  unsigned long now;

  switch (NTPState_) {
    case NTP_Waiting:
      ntpPinHigh = false;
      break;
    case NTP_Connecting:
      now = millis();
      if (now - lastBlinkTime > NTP_BLINK_MS) {
        ntpPinHigh = !ntpPinHigh;
        lastBlinkTime = now;
      }
      break;
    case NTP_Connected:
      ntpPinHigh = true;
      break;
    default:
      break;
  }

  digitalWrite(NTP_STATUS_PIN, ntpPinHigh ? HIGH : LOW);
}

void IotConfig::setup() {
  Serial.println("=IotConfig::setup()");
  if (initialized_) {
    Serial.println("[WARN] Trying to setup IotConfig multiple times.");
    return;
  }

  // Getting NTP time is not in iot_web_conf. So pin config here
  pinMode(NTP_STATUS_PIN, OUTPUT);
  updateNTPLEDStatus_();

  // Here we can set IotWebConf's status and config reset pins, if available.
  // iot_web_conf_.setStatusPin(LEDC_PIN);
  iot_web_conf_.setConfigPin(IOT_AP_CONFIG_PIN);
  iot_web_conf_.setStatusPin(IOT_WEB_STATUS_PIN);

  iot_web_conf_.addParameter(&datetime_separator_);
  // iot_web_conf_.addParameter(&date_param_);
  // iot_web_conf_.addParameter(&time_param_);
  // iot_web_conf_.addParameter(&dst_param_);
  iot_web_conf_.addParameter(&timezone_param_);
  iot_web_conf_.addParameter(&display_separator_);
  iot_web_conf_.addParameter(&show_ampm_param_);
  iot_web_conf_.addParameter(&ldr_sensitivity_param_); 
//  iot_web_conf_.addParameter(&palette_id_param_);
  iot_web_conf_.addParameter(&color_param_);
//  iot_web_conf_.addParameter(&period_param_);
  iot_web_conf_.addParameter(&test_separator_);
  iot_web_conf_.addParameter(&clock_mode_param_);
  iot_web_conf_.addParameter(&find_word_param_);
  // iot_web_conf_.addParameter(&fast_time_factor_param_);

  iot_web_conf_.setConfigSavedCallback([this]() {
    handleConfigSaved_();
  });
  iot_web_conf_.setWifiConnectionCallback([this]() {
    handleWifiConnected_();
  });
  iot_web_conf_.setHtmlFormatProvider(&customHtmlFormatProvider);

  iot_web_conf_.init();

  clearTransientParams_();
  updateClockFromParams_();

  web_server_.on("/", [this]() {
    handleHttpToRoot_();
  });
  web_server_.on("/config", [this]() {
    handleHttpToConfig_();
  });
  web_server_.onNotFound([this]() {
    iot_web_conf_.handleNotFound();
  });

  initialized_ = true;
}

void IotConfig::loop() {
  unsigned long now;

  if (!initialized_) {
    Serial.println("[ERROR] IotConfig not initialized, loop aborted.");
    return;
  }

  now = millis();
  // Do a regular NTP re-connect as long as we're not connected to the NTP server
  if (NTPState_ == NTP_Connecting &&
      now - lastNTPConnect_ > NTP_POLL_SHORT_DELAY_SECONDS * 1000) {
    connectNTP_();
  }
  // Every so many hours, we want to reconnect to the NTP server
  if (now - lastNTPConnect_ > NTP_POLL_LONG_DELAY_SECONDS * 1000) {
    connectNTP_();
  }

  updateNTPLEDStatus_(); // controls the LED pin
  if (NTPState_ == NTP_Connecting) {
    if (isNTPConnected_()) {
      Serial.println("NTP connection succesful");
      NTPState_ = NTP_Connected;
    }
  }

  iot_web_conf_.doLoop();
}
