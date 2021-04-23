/*
 * Blynk-клиент
 *
 * Версия:    0.1
 * Дата:      22.04.2021
 * Автор:     bankviktor14@gmail.com
 */

#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <ModbusMaster.h>
#include "dps.h"
#include "secret.h"
#include "../../Shared/include/commands.h"


#if !defined(SECRET__H) || !defined(AUTH) || !defined(SSID) || !defined(PASS)
#error "secret.h file not found. Create the file and define AUTH/SSID/PASS in file."  
#endif


/* Private Defines ------------------------------------------------------------ */

#define UART_SPEED                  9600  // скорость соединения
#define DPS_UPDATE_INTERVAL_MS      1000  // ms   
#define SELECT_VOLTAGE_MAX_LOWER    2     // ниже на 2V

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
#define PIN_CHARGING_LED            V9    // индикатор включения зарядки устройства 
#define PIN_CHARGING                V10   // включение зарядки устройства
#define PIN_ACCUMULATOR_LED         V11   // индикатор подключения аккумулятора к устройству
#define PIN_ACCUMULATOR             V12   // подключение аккумулятора к устройству
#define PIN_DEVICE_1_LED            V13   // индикатор устройства #1
#define PIN_DEVICE_2_LED            V14   // индикатор устройства #2
#define PIN_TERMINAL                V20   // Лог-терминал


/* Private Types -------------------------------------------------------------- */

enum class DeviceIndex {
  DPS_1 = 1,  // DPS 5020 
  DPS_2 = 2,  // DPS 5015
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
int fCharging_1 = 0;                              // флаг зарядки устройства #1
int fCharging_2 = 0;                              // флаг зарядки устройства #2
int fAccumulator_1 = 0;                           // флаг подключения аккумулятора к устройству #1
int fAccumulator_2 = 0;                           // флаг подключения аккумулятора к устройству #2
DeviceIndex curDev = DeviceIndex::DPS_1;          // текущие устройство
WidgetTerminal terminal(PIN_TERMINAL);
WidgetLED ledPowerSupply(PIN_POWER_SUPPLY_LED);
WidgetLED ledCharging(PIN_CHARGING_LED);
WidgetLED ledAccumulator(PIN_ACCUMULATOR_LED);
WidgetLED ledDevice_1(PIN_DEVICE_1_LED);
WidgetLED ledDevice_2(PIN_DEVICE_2_LED);
DPSData data;
ModbusMaster modBus;                              // ModBus соединение
BlynkTimer timer;


/* Private Function Declarations ---------------------------------------------- */

void selectDevice(DeviceIndex dev);
void parseCmdResponce(const String &resp);
void updateDataCallback();
void toggleCharging(int *pfCharging);
void updateLeds();
void printError(char letter, uint8_t errorCode);
void setMaxVoltage(double max);
void setMaxCurrent(int max);


/* Private Callback Functions ------------------------------------------------- */

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
  int stage = param.asInt();
  if (stage)
  {
    Serial.println(curDev == DeviceIndex::DPS_1 ? F(CMD_SELECT_DPS_2) : F(CMD_SELECT_DPS_1));
  }
}

// напряжение входное
BLYNK_READ(PIN_VOLTAGE_IN)
{
  double value = (double)data.voltage_in / 100;
  Blynk.virtualWrite(PIN_VOLTAGE_IN, value);
  setMaxVoltage(value);
}

// напряжение выход/аккум
BLYNK_READ(PIN_VOLTAGE_OUT_OR_ACC)
{
  double value = (double)data.voltage_out / 100;
  Blynk.virtualWrite(PIN_VOLTAGE_OUT_OR_ACC, value);
}

// мощность
BLYNK_READ(PIN_POWER)
{
  double value = data.power_out < 1000 
    ? ((double)data.power_out / 100) 
    : ((double)data.power_out / 10);
  Blynk.virtualWrite(PIN_POWER, value);
}

// ток выходной
BLYNK_READ(PIN_CURRENT_OUT)
{
  double value = (double)data.current_out / 100;
  Blynk.virtualWrite(PIN_CURRENT_OUT, value);
}

// установка напряжения выходного
BLYNK_WRITE(PIN_SELECT_VOLTAGE)
{
  int value = param.asInt();
  uint8_t result = modBus.writeSingleRegister(DPS_REG_VOLTAGE_SET, value * 100);
  if (result != modBus.ku8MBSuccess)
  {
    printError('A', result);
  }
}

// установка тока выходного
BLYNK_WRITE(PIN_SELECT_CURRENT)
{
  int value = param.asInt();
  uint8_t result = modBus.writeSingleRegister(DPS_REG_CURRENT_SET, value * 100);
  if (result != modBus.ku8MBSuccess)
  {
    printError('B', result);
  }
}

// включение/выключение зарядки
BLYNK_WRITE(PIN_CHARGING)
{
  int stage = param.asInt();
  if (stage)
  {
    toggleCharging(curDev == DeviceIndex::DPS_1 ? &fCharging_1 : &fCharging_2);
  }
}

// подключение/отключение аккумулятора к устройству
BLYNK_WRITE(PIN_ACCUMULATOR)
{
  int stage = param.asInt();
  if (stage)
  {
    if (curDev == DeviceIndex::DPS_1)
    {
      Serial.println(fAccumulator_1 ? F(CMD_ACCUMULATOR_1_OFF) : F(CMD_ACCUMULATOR_1_ON));
    }
    else
    {
      Serial.println(fAccumulator_2 ? F(CMD_ACCUMULATOR_2_OFF) : F(CMD_ACCUMULATOR_2_ON));
    }
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
  ledPowerSupply.off();
  ledDevice_1.on();
  ledDevice_2.off();
  ledCharging.off();
  ledAccumulator.off();

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
  auto result = modBus.readHoldingRegisters(DPS_REG_VOLTAGE_SET, 10);
  if (result == modBus.ku8MBSuccess)
  {
    for (int i = 0; i < 10; i++)
    {
      *((uint16_t*)&data + i) = modBus.getResponseBuffer(i);
    }
  }
}


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
    updateLeds();
    setMaxVoltage(DPS_1_CURRENT_MAX);
  }
  else if (resp == F(RESPONCE_SELECT_DEVICE_2)) 
  {
    curDev = DeviceIndex::DPS_2;
    updateLeds();
    setMaxCurrent(DPS_2_CURRENT_MAX);
  }
  else if (resp == F(RESPONCE_ACCUMULATOR_1_ON)) 
  {
    fAccumulator_1 = 1;
    updateLeds();
  }
  else if (resp == F(RESPONCE_ACCUMULATOR_1_OFF)) 
  {
    fAccumulator_1 = 0;
    updateLeds();
  }
  else if (resp == F(RESPONCE_ACCUMULATOR_2_ON)) 
  {
    fAccumulator_2 = 1;
    updateLeds();
  }
  else if (resp == F(RESPONCE_ACCUMULATOR_2_OFF)) 
  {
    fAccumulator_2 = 0;
    updateLeds();
  }
}


void setMaxVoltage(double max)
{
  Blynk.setProperty(PIN_SELECT_VOLTAGE, F("max"), (int)(max - SELECT_VOLTAGE_MAX_LOWER));
  // if (data.current_set < max * 100)
  // {
  //   uint8_t result = modBus.writeSingleRegister(DPS_REG_VOLTAGE_SET, max * 100);
  //   if (result != modBus.ku8MBSuccess)
  //   {
  //     printError('C', result);
  //   }
  // }
}


void setMaxCurrent(int max)
{
  Blynk.setProperty(PIN_SELECT_CURRENT, F("max"), max);
  // if (data.current_set < max * 100)
  // {
  //   uint8_t result = modBus.writeSingleRegister(DPS_REG_CURRENT_SET, max * 100);
  //   if (result != modBus.ku8MBSuccess)
  //   {
  //     printError('D', result);
  //   }
  // }
}


void updateLeds()
{
  if (curDev == DeviceIndex::DPS_1)
  {
    ledDevice_1.on();
    ledDevice_2.off();

    if (fCharging_1) ledCharging.on(); else ledCharging.off();
    if (fAccumulator_1) ledAccumulator.on(); else ledAccumulator.off();
  }
  else if (curDev == DeviceIndex::DPS_2)
  {
    ledDevice_1.off();
    ledDevice_2.on();

    if (fCharging_2) ledCharging.on(); else ledCharging.off();
    if (fAccumulator_2) ledAccumulator.on(); else ledAccumulator.off();
  }
}


void toggleCharging(int *pfCharging)
{
  uint16_t newValue = !(*pfCharging);
  uint8_t result = modBus.writeSingleRegister(DPS_REG_ONOFF, newValue);
  if (result == modBus.ku8MBSuccess) 
  {
    *pfCharging = newValue;
    if (newValue) 
    {
      ledCharging.on();
    }
    else
    {
      ledCharging.off();
    }
  }
  else
  {
    printError('E', result);
  }
}


void printError(char letter, uint8_t errorCode)
{
  terminal.printf("ModBus Error %c-%X\n", letter, errorCode);
  terminal.flush();
}
