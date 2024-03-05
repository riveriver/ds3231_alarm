#include <Arduino.h>
#include "DS3231.h"
// #include "RealTimeClock.h"
// RealTimeClock rtc_c;

#define IO_WIRE0_SDA 8
#define IO_WIRE0_SCL 9
TwoWire wire_0 = TwoWire(0);
DS3231 rtc_date_time;
void initSystem() {
  // pinMode(1, OUTPUT);
  // digitalWrite(1, HIGH);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial System Begin !");
}

void iic_scan(TwoWire *wire){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ ){
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    wire->beginTransmission(address);
    error = wire->endTransmission();
    if (error == 0){
      Serial.print("I2C device found at address 0x");
      if (address < 16)Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }else if (error == 4){
      Serial.print("Unknow error at address 0x");
      if (address < 16)Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("I2C scan done\n");
}

ICACHE_RAM_ATTR void alarm_interrupt(){
  Serial.println("alarm_interrupt !");
};

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;//由外部信号使用RTC_IO引起的唤醒
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;//使用RTC_CNTL的外部信号引起的唤醒
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;//由计时器引起的唤醒
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;//由触摸板引起的唤醒
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;//由触摸板引起的唤醒
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;//由触摸板引起的唤醒
  }
}

// 将数据存储在 RTC 内存中，以便在重新启动时使用
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int bootFlag = 0;

void setup() {
  initSystem();
  wire_0.begin(IO_WIRE0_SDA, IO_WIRE0_SCL, 400000U);
  iic_scan(&wire_0);
  rtc_date_time.init(&wire_0);
  pinMode(10,INPUT_PULLUP); 
  interrupts();
  attachInterrupt(digitalPinToInterrupt(10),alarm_interrupt,FALLING); 
  pinMode(4,INPUT_PULLUP); 
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);
   print_wakeup_reason();
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  if(bootCount ==  1){
    rtc_date_time.clearAlarm(1);
    rtc_date_time.disableAlarm(1);
    DateTime now_time(24,3,4,16,10,58);
    rtc_date_time.adjust(now_time);
    DateTime alarm_time(24,3,4,16,11,12);
    if(rtc_date_time.setAlarm1(alarm_time,DS3231_A1_Minute) == false){
        Serial.printf("[ERROR]setAlarm1\n");
    }
  }
  rtc_date_time.update();
}

void loop() {
  if(bootFlag == false){
    bootFlag = true;
    Serial.println("Going to sleep now");
    delay(1000);
    esp_deep_sleep_start();
  }
}


