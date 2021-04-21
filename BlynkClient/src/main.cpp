#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <ModbusMaster.h>
#include "dps.h"
#include "secret.h"

#if !defined(SECRET__H) || !defined(AUTH) || !defined(SSID) || !defined(PASS)
#error "secret.h file not found. Create the file and define AUTH/SSID/PASS in file."  
#endif

// DPS 5020 0-50V  0-20A
// DPS 5015 0-50V  0-15A

/* Private Defines ------------------------------------------------------------ */

#define UART_SPEED                  9600  // скорость соединения
#define DPS_UPDATE_INTERVAL_MS      1000  // ms   

// Virtual Pins 

#define PIN_POWER_SUPPLY_LED        V0    // индикатор питания
#define PIN_POWER_SUPPLY            V1    // переключение питания
#define PIN_SELECT_DEVICE           V2    // выбор устройства
#define PIN_VOLTAGE_IN              V3    // напряжение входное
#define PIN_VOLTAGE_OUT_OR_ACC      V4    // напряжение установка/выход/аккум
#define PIN_POWER                   V5    // мощность
#define PIN_CURRENT_OUT             V6    // ток выходной
#define PIN_SELECT_VOLTAGE          V7    // установка напряжения выходного
#define PIN_SELECT_CURRENT          V8    // установка ток выходного
#define PIN_CHARGING_1_LED          V9    // индикатор включения зарядки устройства #1
#define PIN_CHARGING_1              V10   // включение зарядки устройства #1
#define PIN_CHARGING_2_LED          V11   // индикатор включения зарядки устройства #2
#define PIN_CHARGING_2              V12   // включение зарядки устройства #2
#define PIN_ACCUMULATOR_1_LED       V13   // индикатор подключения аккумулятора к устройству #1
#define PIN_ACCUMULATOR_1           V14   // подключение аккумулятора к устройству #1
#define PIN_ACCUMULATOR_2_LED       V15   // индикатор подключения аккумулятора к устройству #2
#define PIN_ACCUMULATOR_2           V16   // подключение аккумулятора к устройству #2
#define PIN_TERMINAL                V20   // Лог-терминал

// Command Requests

#define CMD_POWER_ON                "fanon"
#define CMD_POWER_OFF               "fanoff"
#define CMD_SELECT_DPS_1            "dps1"      // DPS 5020
#define CMD_SELECT_DPS_2            "dps2"      // DPS 5015
#define CMD_STATUS                  "status"
#define CMD_CHARGING_1_ON           "block1on"
#define CMD_CHARGING_1_OFF          "block1off"
#define CMD_CHARGING_2_ON           "block2on"
#define CMD_CHARGING_2_OFF          "block2off"

// Command Responces

#define RESPONCE_POWER_ON           "FAN:ON"
#define RESPONCE_POWER_OFF          "FAN:OFF"
#define RESPONCE_SELECT_DEVICE_1    "UART:DPS1"
#define RESPONCE_SELECT_DEVICE_2    "UART:DPS2"
#define RESPONCE_BLOCK_1_ON         "BLOCK_1:ON"
#define RESPONCE_BLOCK_1_OFF        "BLOCK_1:OFF"
#define RESPONCE_BLOCK_2_ON         "BLOCK_2:ON"
#define RESPONCE_BLOCK_2_OFF        "BLOCK_2:OFF"


#define DPS_1_CURRENT_MAX           20 // A
#define DPS_2_CURRENT_MAX           15 // A


/* Private Types -------------------------------------------------------------- */

enum class DeviceIndex {
  DPS_1 = 1,  // DPS 5015
  DPS_2 = 2,  // DPS 5020
};

struct DPSData {
  uint16_t voltage_set;   // U-SET    RW    Voltage setting
  uint16_t current_set;   // I-SET    RW    Current setting
  uint16_t voltage_out;   // UOUT     R     Output voltage display value
  uint16_t current_out;   // IOUT     R     Output current display value
  uint16_t power_out;     // POWER    R     Output power display value
  uint16_t voltage_in;    // UIN      R     Input voltage display value
  uint16_t lock;          // LOCK     RW    Key lock
  uint16_t protect;       // PROTECT  R     Protection status
  uint16_t constant;      // CV/CC    R     Constant Voltage / Constant Current status
  uint16_t onoff;         // ONOFF    RW    Switch output state
};


/* Private Variables ---------------------------------------------------------- */

int fPower = 0;                                   // флаг питания устройств
int fCharging_1 = 0;                              // 
int fCharging_2 = 0;                              // 
int fAccumulator_1 = 0;                           // флаг подключения аккумулятора к устройству #1
int fAccumulator_2 = 0;                           // флаг подключения аккумулятора к устройству #2
DeviceIndex curDev = DeviceIndex::DPS_1;          // Текущие устройство
WidgetTerminal terminal(PIN_TERMINAL);
WidgetLED ledPowerSupply(PIN_POWER_SUPPLY_LED);
WidgetLED ledCharging_1(PIN_CHARGING_1_LED);
WidgetLED ledCharging_2(PIN_CHARGING_2_LED);
WidgetLED ledAccumulator_1(PIN_ACCUMULATOR_1_LED);
WidgetLED ledAccumulator_2(PIN_ACCUMULATOR_2_LED);
DPSData dps1;
DPSData dps2;
ModbusMaster modBus; // ModBus соединение
BlynkTimer timer;

/* Private Function Declarations ---------------------------------------------- */

void selectDevice(DeviceIndex dev);
void parseCmdResponce(const String &resp);
void updateDataCallback();

/* Callback Functions --------------------------------------------------------- */

// включение питания
BLYNK_WRITE(PIN_POWER_SUPPLY)
{
  int state = param.asInt();
  if (state)
  {
    Serial.println(fPower ? F(CMD_POWER_OFF) : F(CMD_POWER_ON));
  }
}

// выбор устройства
BLYNK_WRITE(PIN_SELECT_DEVICE)
{
  auto devIndex = (DeviceIndex)param.asInt();
  selectDevice(devIndex);
}

// напряжение входное
BLYNK_READ(PIN_VOLTAGE_IN)
{
  DPSData &data = curDev == DeviceIndex::DPS_1 ? dps1 : dps2;
  double value = (double)data.voltage_in / 100;
  Blynk.virtualWrite(PIN_VOLTAGE_IN, value);
}

// напряжение выход/аккум
BLYNK_READ(PIN_VOLTAGE_OUT_OR_ACC)
{
  DPSData &data = curDev == DeviceIndex::DPS_1 ? dps1 : dps2;
  double value = (double)data.voltage_out / 100;
  Blynk.virtualWrite(PIN_VOLTAGE_OUT_OR_ACC, value);
}

// мощность
BLYNK_READ(PIN_POWER)
{
  DPSData &data = curDev == DeviceIndex::DPS_1 ? dps1 : dps2;
  double value = data.power_out < 1000 ? ((double)data.power_out / 100) : ((double)data.power_out / 10);
  Blynk.virtualWrite(PIN_POWER, value);
}

// ток выходной
BLYNK_READ(PIN_CURRENT_OUT)
{
  DPSData &data = curDev == DeviceIndex::DPS_1 ? dps1 : dps2;
  double value = (double)data.current_out / 100;
  Blynk.virtualWrite(PIN_CURRENT_OUT, value);
}

// установка напряжения выходного
BLYNK_WRITE(PIN_SELECT_VOLTAGE)
{
  int value = param.asInt();
  modBus.writeSingleRegister(DPS_REG_VOLTAGE_SET, value * 100);
}

// установка тока выходного
BLYNK_WRITE(PIN_SELECT_CURRENT)
{
  int value = param.asInt();
  modBus.writeSingleRegister(DPS_REG_CURRENT_SET, value * 100);
}

// включение/выключение зарядки устройства #1
BLYNK_WRITE(PIN_CHARGING_1)
{
  int stage = param.asInt();
  if (stage)
  {
    
  }
}

// включение/выключение зарядки устройства #2
BLYNK_WRITE(PIN_CHARGING_2)
{
  int stage = param.asInt();
  if (stage)
  {
    
  }
}

// подключение/отключение аккумулятора к устройству #1
BLYNK_WRITE(PIN_ACCUMULATOR_1)
{
  int stage = param.asInt();
  if (stage)
  {
    Serial.println(fAccumulator_1 ? F(RESPONCE_BLOCK_1_OFF) : F(RESPONCE_BLOCK_1_ON));
  }
}

// подключение/отключение аккумулятора к устройству #2
BLYNK_WRITE(PIN_ACCUMULATOR_2)
{
  int stage = param.asInt();
  if (stage)
  {
    Serial.println(fAccumulator_2 ? F(RESPONCE_BLOCK_2_OFF) : F(RESPONCE_BLOCK_2_ON));
  }
}

// терминал
BLYNK_WRITE(PIN_TERMINAL)
{
  String str = param.asStr();
  Serial.println(str);
}


/* Private Function Definitions ----------------------------------------------- */

void setup()
{
  Serial.begin(UART_SPEED);
  Blynk.begin(AUTH, SSID, PASS);

  // Инициализация
  Blynk.virtualWrite(PIN_SELECT_DEVICE, (int)curDev);
  ledPowerSupply.off();

  // Blynk терминал
  terminal.clear();
  terminal.println(F("Blynk ready"));
  terminal.flush();

  modBus.begin(1, Serial);

  timer.setInterval(DPS_UPDATE_INTERVAL_MS, updateDataCallback);
}


void loop()
{
  Blynk.run();
  timer.run(); 

  if (Serial.available())
  {
    String cmdResp = Serial.readStringUntil('\r');

    cmdResp.trim();

    if (cmdResp[cmdResp.length() - 1] == '\r')
    {
      cmdResp.remove(cmdResp.length() - 1);
    }

    // Echo в Blynk терминал
    terminal.print(F("rx>"));
    terminal.println(cmdResp);
    terminal.flush();

    parseCmdResponce(cmdResp);
  }
}


void updateDataCallback()
{
  DPSData &data = curDev == DeviceIndex::DPS_1 ? dps1 : dps2;
  
  auto result = modBus.readHoldingRegisters(0x0000, 10);
  if (result == modBus.ku8MBSuccess)
  {
    for (int i = 0; i < 10; i++)
    {
      *((uint16_t*)&data + i) = modBus.getResponseBuffer(i);
    }
  }
  else
  {
    terminal.print("modBus error code ");
    terminal.println(result);
    terminal.flush();
  }
}

// Парсит ответ на команду и меняет состояние
void parseCmdResponce(const String &resp)
{
  if (resp == F(RESPONCE_POWER_ON)) 
  {
    fPower = 1;
    ledPowerSupply.on();
  }
  else if (resp == F(RESPONCE_POWER_OFF))
  {
    fPower = 0;
    ledPowerSupply.off();
  }
  else if (resp == F(RESPONCE_SELECT_DEVICE_1)) 
  {
    curDev = DeviceIndex::DPS_1;
    Blynk.virtualWrite(PIN_SELECT_DEVICE, (int)curDev);
    Blynk.setProperty(PIN_SELECT_CURRENT, F("max"), DPS_1_CURRENT_MAX);
    if (dps1.current_set < DPS_1_CURRENT_MAX * 100)
    {
      modBus.writeSingleRegister(DPS_REG_CURRENT_SET, DPS_1_CURRENT_MAX * 100);
    }
  }
  else if (resp == F(RESPONCE_SELECT_DEVICE_2)) 
  {
    curDev = DeviceIndex::DPS_2;
    Blynk.virtualWrite(PIN_SELECT_DEVICE, (int)curDev);
    Blynk.setProperty(PIN_SELECT_CURRENT, F("max"), DPS_2_CURRENT_MAX);
    if (dps2.current_set < DPS_2_CURRENT_MAX * 100)
    {
      modBus.writeSingleRegister(DPS_REG_CURRENT_SET, DPS_2_CURRENT_MAX * 100);
    }
  }
  else if (resp == F(RESPONCE_BLOCK_1_ON)) 
  {
    fAccumulator_1 = 1;
    ledAccumulator_1.on();
  }
  else if (resp == F(RESPONCE_BLOCK_1_OFF)) 
  {
    fAccumulator_1 = 0;
    ledAccumulator_1.off();
  }
  else if (resp == F(RESPONCE_BLOCK_2_ON)) 
  {
    fAccumulator_2 = 1;
    ledAccumulator_2.on();
  }
  else if (resp == F(RESPONCE_BLOCK_2_OFF)) 
  {
    fAccumulator_2 = 0;
    ledAccumulator_2.off();
  }
}


void selectDevice(DeviceIndex dev)
{
  const __FlashStringHelper* cmd;

  switch (dev)
  {
  case DeviceIndex::DPS_1:
    cmd = F(CMD_SELECT_DPS_1);
    break;
  
  case DeviceIndex::DPS_2:
    cmd = F(CMD_SELECT_DPS_2);
    break;

  default:
    return;
  }

  Serial.println(cmd);
}
