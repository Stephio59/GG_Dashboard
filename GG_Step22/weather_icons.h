/*
 * weather_icons.h - Mapping codes OWM -> nom icone BMP sur SD
 */
#pragma once

// Convertit un code OpenWeatherMap (ex "01d", "10n") en nom d'icone SD
// Codes OWM :
//   01x = ciel clair    -> weather_sunny
//   02x = peu nuageux   -> weather_partly_cloudy
//   03x = nuageux       -> weather_cloudy
//   04x = tres nuageux  -> weather_cloudy
//   09x = pluie averse  -> weather_rain
//   10x = pluie         -> weather_rain
//   11x = orage         -> weather_storm
//   13x = neige         -> weather_snow
//   50x = brouillard    -> weather_fog
inline const char* owm_to_icon(const char* owm_code) {
    if (!owm_code || !owm_code[0]) return "weather_sunny";
    if (owm_code[0] == '0') {
        char d = owm_code[1];
        if (d == '1') return "weather_sunny";
        if (d == '2') return "weather_partly_cloudy";
        if (d == '3' || d == '4') return "weather_cloudy";
        if (d == '9') return "weather_rain";
    }
    if (owm_code[0] == '1') {
        if (owm_code[1] == '0') return "weather_rain";
        if (owm_code[1] == '1') return "weather_storm";
        if (owm_code[1] == '3') return "weather_snow";
    }
    if (owm_code[0] == '5') return "weather_fog";
    return "weather_sunny";  // fallback
}
