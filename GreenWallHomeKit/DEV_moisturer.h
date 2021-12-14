struct DEV_moisturer : Service::HumidifierDehumidifier {
  int moistureRead;
  int moisturePower;
  int pumpPower;

  const int maxMoisture = 1200; //value for 3V input, 5V input generate at least 1400mV output
  const int minMoisture = 3400; //value for 3V input, 5V input generate at least 3700mV output
  const int WET = 2000;
  const int DRY = 3000;

  SpanCharacteristic *power;
  SpanCharacteristic *currentMoisture;
  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;

  const int moistureCheckInterval = 3 * 3600000; //3 hours
//  const int wateringLongInterval = 48 * 3600000; //48 hours
  const int wateringShortInterval = 60 * 1000; //minute
  unsigned long previousTime = 0;

  DEV_moisturer(int moisturerPowerPin, int moisturerReadPin, int pumpPowerPin) : Service::HumidifierDehumidifier() {
    pinMode(moisturerPowerPin, OUTPUT);
    pinMode(moisturerReadPin, INPUT);
    pinMode(pumpPowerPin, OUTPUT);
    
    LOG1("Power and Read pin initialized.\n");

    digitalWrite(moisturerPowerPin, HIGH);
    delay(3000);
    double moist= this->moistureInPerc(analogRead(moisturerReadPin));
    digitalWrite(moisturerPowerPin, LOW);

    LOG1("MOISTURE IS: ");
    LOG1(moist);
    
    power = new Characteristic::Active(1);
    currentMoisture = new Characteristic::CurrentRelativeHumidity(moist);
    currentState = new Characteristic::CurrentHumidifierDehumidifierState(1); // 0 (inactive-The accesory is off), 1 (idle-The accessory is idle), 2 (humidifying-The accessory is adding water to the air.), 3 (dehumidifying-The accessory is removing water from the air.)
    targetState = new Characteristic::TargetHumidifierDehumidifierState(0); // 0 (automatic-The accessory should decide whether to add or remove water to or from the air.), 1 (humidify-The accessory should add water to the air.), 2 (dehumidify-The accessory should remove water from the air.)
    //optional characteristics
    new Characteristic::Name("Bananowiec");

    LOG1("\nRequired characteristics initialized.\n");
    
    this->moisturePower=moisturerPowerPin;
    this->moistureRead=moisturerReadPin;
    this->pumpPower=pumpPowerPin;

    LOG1("Moisture power, read and pump attributes initialized.\n");
  }

  double moistureInPerc(int currentReading) {
    return ((((double)(currentReading-maxMoisture)/(double)(minMoisture-maxMoisture))-1)*-100);
  }

  void checkMoisture() {
    double data[5];
    int i=0;
    int sum=0;
    double mean=0.0;
    
    digitalWrite(moisturePower, HIGH);
    delay(3000);
    for (i=0; i<5; ++i) {
      data[i]=this->moistureInPerc(analogRead(this->moistureRead));
//      data[i]=analogRead(this->moistureRead);
      LOG1(i);
      LOG1(": ");
      LOG1(data[i]);
      LOG1("; ");
      delay(500); 
    }
    digitalWrite(moisturePower, LOW);
  
    double standardDeviation=this->calculateSD(data);
    LOG1("\nStandard Deviation: ");
    LOG1(standardDeviation);
    LOG1("\n");
    for (i=0; i<5; ++i) {
      sum+=data[i];
    }
    mean=sum/5;
    
    for (i=0; i<5; ++i) {
      if (abs(mean-data[i])<standardDeviation) {
        currentMoisture->setVal(data[i]);
        LOG1("Choosen value: ");
        LOG1(currentMoisture->getVal());
        LOG1("\n");
        break;
      }
    }
  }

  double calculateSD(double data[]){
    double sum = 0.0;
    double mean = 0.0;
    double SD = 0.0;
    int i;
    
    for (i = 0; i < 5; ++i) {
        sum += float(data[i]);
    }
    mean = sum / 5;
    for (i = 0; i < 5; ++i) {
        SD += pow(data[i] - mean, 2);
    }
    return sqrt(SD / 5);
  }

  void rehydrateCrops() {
    LOG1("Watering crops in progress....\n");
    digitalWrite(this->pumpPower, HIGH);
    delay(10000);
    digitalWrite(this->pumpPower,LOW);
    LOG1("Watering crops done!\n");
  }

  void powerOn() {
    LOG1("Requested switch ON the device.\n");
    power->setVal(1);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  void powerOff() {
    LOG1("Requested switch OFF the device.\n");
    power->setVal(0);
    digitalWrite(LED_BUILTIN, LOW);
  }

  boolean update() {
    int state = targetState->getNewVal();
    switch (state) {
      case 0: //automatic
        LOG1("Requested automatic watering mode.\n");
        currentState->setVal(1); // idle
        break;
      case 1: //watering
        LOG1("Requested watering.\n");
        currentState->setVal(2); // watering
        LOG1("Watering crops in progress....\n");
        digitalWrite(this->pumpPower, HIGH);
        this->previousTime = millis();
        break;
      case 2: //unsupported
        LOG1("Unsupported mode - ignoring.\n");
        targetState->setVal(0); //automatic
        break;
      default:
        LOG1("UNDEFINED mode requested!!!");
        LOG1(state);
        LOG1("\n");
        targetState->setVal(0); //automatic
        break;
    }

    if (power->getNewVal()==0){
      this->powerOff();
    } else {
      this->powerOn();
    }

    return (true);
  }
  

  void loop() {
    unsigned long currentTime = millis();
    if (currentMoisture->timeVal()>15000) this->checkMoisture();
//    if(currentMoisture->timeVal()>this->moistureCheckInterval) this->checkMoisture();
    if (currentState->getVal()==2 && currentTime-this->previousTime>10000) {
      digitalWrite(this->pumpPower,LOW);
      LOG1("Watering crops done!\n");
      currentState->setVal(1); // idle
      targetState->setVal(0); // automatic
    }

    if (targetState->getVal()==0 && currentMoisture->getVal()<=72) {
      if (currentTime-this->previousTime>=this->wateringShortInterval) {
        this->previousTime = millis();
        currentState->setVal(2);
        LOG1("Watering crops in progress....\n");
        digitalWrite(this->pumpPower, HIGH);
      }      
    }
  }
};
