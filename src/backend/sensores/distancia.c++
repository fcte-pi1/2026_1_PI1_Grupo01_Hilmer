#define SENSOR_LEFT   34
#define SENSOR_FRONT  35
#define SENSOR_RIGHT  36
#define FILTER_SIZE 1

struct SensorParede {
  int pin;
  int readings[FILTER_SIZE];
  int index;
  bool wallDetected;
};

SensorParede sensorEsq    = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir    = {SENSOR_RIGHT};

void initSensor(SensorParede &sensor) {
  sensor.index = 0;
  sensor.wallDetected = false;

  for (int i = 0; i < FILTER_SIZE; i++) {
    sensor.readings[i] = HIGH;
  }
}

bool lerSensorFiltrado(SensorParede &sensor) {
  int value = digitalRead(sensor.pin);

  sensor.readings[sensor.index] = value;
  sensor.index = (sensor.index + 1) % FILTER_SIZE;

  int sum = 0;

  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += sensor.readings[i];
  }

  float average = sum / (float)FILTER_SIZE;
  return average < 0.5;
}

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_LEFT, INPUT_PULLUP);
  pinMode(SENSOR_FRONT, INPUT_PULLUP);
  pinMode(SENSOR_RIGHT, INPUT_PULLUP);

  initSensor(sensorEsq);
  initSensor(sensorFrente);
  initSensor(sensorDir);
}

void loop() {
  sensorEsq.wallDetected = lerSensorFiltrado(sensorEsq);
  sensorFrente.wallDetected = lerSensorFiltrado(sensorFrente);
  sensorDir.wallDetected = lerSensorFiltrado(sensorDir);

  Serial.print(" | FRENTE: ");
  Serial.print(sensorFrente.wallDetected ? "PAREDE" : "LIVRE");
  Serial.print(" | direita: ");
  Serial.print(sensorDir.wallDetected ? "PAREDE" : "LIVRE");
  Serial.print(" | esquerda: ");
  Serial.print(sensorEsq.wallDetected ? "PAREDE" : "LIVRE");
  Serial.print("\n");

  delay(30);
}