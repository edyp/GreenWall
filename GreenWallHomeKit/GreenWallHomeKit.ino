#include "HomeSpan.h" 
#include "DEV_moisturer.h"


void setup() {
  Serial.begin(115200);

  setCpuFrequencyMhz(80);
  
  homeSpan.setStatusPin(LED_BUILTIN);
  homeSpan.setControlPin(15);
  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Humidifiers, "SmartGrow");
  
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Inteligentna doniczka");
      new Characteristic::Manufacturer("e-pajak");
      new Characteristic::SerialNumber("123123");
      new Characteristic::Model("Podlewacz");
      new Characteristic::FirmwareRevision("0.1");
      new Characteristic::Identify();
    
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");

    new DEV_moisturer(12, 36, 14);
}

void loop() {
  homeSpan.poll();
}
