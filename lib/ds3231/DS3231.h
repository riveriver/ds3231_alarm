#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"

class DS3231 : public RTC_DS3231{
private:
public:
    byte init(TwoWire *wire){
       if (!begin(wire)) {return 1;}


       return 0;
    }
    byte update(){
        DateTime now_time = now();
        printDateTime(now_time);
        // float temp = getTemperature();
        // Serial.printf("Temperature:%f\n",temp);
        DateTime alarm_set = getAlarm1();
        printDateTime(alarm_set);
        // byte sqw = readSqwPinMode();
        // Serial.printf("sqw:%X\n",sqw);
        // byte mode = getAlarm1Mode();
        // Serial.printf("Alarm1Mode:%X\n",mode);
        // byte control = getAlarmControl();
        // Serial.printf("AlarmControl:%X\n",control);
        // for (int i = 7; i >= 0; i--) {
        // Serial.print((control >> i) & 1);
        // }
        // Serial.println();
        byte fired = alarmFired(1);
        Serial.printf("fired:%d\n",fired);
        return 0;
    }
    void printDateTime(const DateTime& dt)
    {
        char str[20];
        snprintf_P(str, 
            sizeof(str),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.month(),
            dt.day(),
            dt.year(),
            dt.hour(),
            dt.minute(),
            dt.second() );
        Serial.print(str);
        Serial.println("");
    }
};
