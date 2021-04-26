/*
 * Blynk-клиент
 *
 * Версия:    0.1
 * Дата:      26.04.2021
 * Автор:     bankviktor14@gmail.com
 *
 */

/* Private Defines ------------------------------------------------------------ */

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_M590
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_A7
//#define TINY_GSM_MODEM_BG96
//#define TINY_GSM_MODEM_XBEE

// Default heartbeat interval for GSM is 60
// If you want override this value, uncomment and set this option:
//#define BLYNK_HEARTBEAT 30

#define LED_ACTIVE                        LOW
#define POWER_TOGGLE_DELAY_MS             1000
#define DPS_UPDATE_INTERVAL_MS            1000
#define DEBUG_SPEED                       9600
#define SIM800_SPEED                      9600
#define SELECT_VOLTAGE_MAX_LOWER          2


/* Private Includes ----------------------------------------------------------- */

#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include "../../Shared/include/commands.h"
#include "secret.h"
#include "pins.h"
#include "dps.h"


#if !defined(SECRET__H) || !defined(BLYNK_AUTH) || !defined(GPRS_APN) || !defined(GPRS_USER) || !defined(GPRS_PASSWORD)
#error "secret.h file not found. Create the file and define BLYNK_AUTH/GPRS_APN/GPRS_USER/GPRS_PASSWORD in file."  
#endif


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

struct BUTTON
{
  bool fState = false;
  bool fIsFirst = false;
};


/* Private Variables ---------------------------------------------------------- */

SoftwareSerial SerialAT(PIN_SIM800_RX, PIN_SIM800_TX); // RX, TX
TinyGsm modem(SerialAT);
ModbusMaster modBus;                              // ModBus соединение

// WidgetTerminal terminal(PIN_TERMINAL);
WidgetLED ledPowerSupply(PIN_POWER_SUPPLY_LED);
WidgetLED ledCharging(PIN_CHARGING_LED);
WidgetLED ledAccumulator(PIN_ACCUMULATOR_LED);
WidgetLED ledDevice_1(PIN_DEVICE_1_LED);
WidgetLED ledDevice_2(PIN_DEVICE_2_LED);
BlynkTimer timer;

BUTTON Button_POWER;
BUTTON Button_ACCUMULATOR_1;
BUTTON Button_ACCUMULATOR_2;

DeviceIndex curDev = DeviceIndex::DPS_1;      // текущие устройство
bool fPower = 0;                              // флаг питания устройств
bool fCharging_1 = 0;                         // флаг зарядки устройства #1
bool fCharging_2 = 0;                         // флаг зарядки устройства #2   
bool fAccumulator_1 = 0;                      // флаг подключения аккумулятора к устройству #1
bool fAccumulator_2 = 0;                      // флаг подключения аккумулятора к устройству #2
DPSData data = { 0 };


/* Private Function Declarations ---------------------------------------------- */

void pinsInit();
void updateDataCallback();
void toggleCharging(bool *pfCharging);
void updateLeds();
void setMaxVoltage(double max);
void setMaxCurrent(int max);
void buttonHandle(int pin, BUTTON* pBtn, const __FlashStringHelper * pszCmd);
void buttonUpdateState(int pin, BUTTON* pBtn);
void doCommand(const String &command, bool isPrint = false);
void printError(char letter, uint8_t errorCode);


/* Callback Functions --------------------------------------------------------- */

// включение питания
BLYNK_WRITE(PIN_POWER_SUPPLY)
{
  int state = param.asInt();
  if (state)
  {
    doCommand(fPower ? F(CMD_POWER_OFF) : F(CMD_POWER_ON));
  }
}

// выбор устройства
BLYNK_WRITE(PIN_SELECT_DEVICE)
{
  int stage = param.asInt();
  if (stage)
  {
    doCommand(curDev == DeviceIndex::DPS_1 ? F(CMD_SELECT_DPS_2) : F(CMD_SELECT_DPS_1));
  }
}

// напряжение входное
BLYNK_READ(PIN_VOLTAGE_IN)
{
  double value = (double)data.voltage_in / 100;
  Blynk.virtualWrite(PIN_VOLTAGE_IN, value);
  if (value > 0)
  {
    setMaxVoltage(value);
  }
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
  Blynk.virtualWrite(PIN_SELECT_VOLTAGE, value);
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
  Blynk.virtualWrite(PIN_SELECT_CURRENT, value);
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
      doCommand(fAccumulator_1 ? F(CMD_ACCUMULATOR_1_OFF) : F(CMD_ACCUMULATOR_1_ON));
    }
    else
    {
      doCommand(fAccumulator_2 ? F(CMD_ACCUMULATOR_2_OFF) : F(CMD_ACCUMULATOR_2_ON));
    }
  }
}

// терминал
// BLYNK_WRITE(PIN_TERMINAL)
// {
//   String str = param.asStr();
//   Serial.println(str);
// }


/* Private Function Definitions ----------------------------------------------- */

void setup()
{
  Serial.begin(DEBUG_SPEED);
  Serial.println(F("Booting..."));

  pinsInit();

  modBus.begin(1, Serial);

  SerialAT.begin(SIM800_SPEED);

  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println(F("Initializing modem..."));
  //modem.restart();
  modem.init();
  //modem.simUnlock("1234");  // Unlock your SIM card with a PIN

  Serial.println(F("Blynk connecting..."));
  Blynk.begin(BLYNK_AUTH, modem, GPRS_APN, GPRS_USER, GPRS_PASSWORD);

  timer.setInterval(DPS_UPDATE_INTERVAL_MS, updateDataCallback);

  ledPowerSupply.off();
  ledDevice_1.on();
  ledDevice_2.off();
  ledCharging.off();
  ledAccumulator.off();

  // // Blynk терминал
  // terminal.clear();
  // terminal.println(F("Blynk ready"));
  // terminal.flush();

  Serial.println(F("Ready"));
}


void loop()
{
  Blynk.run();
  timer.run();

  if (Serial.available())
  {
    String command = Serial.readStringUntil('\r');

    command.trim();

    if (command[command.length() - 1] == '\r')
    {
      command.remove(command.length() - 1);
    }

    // Echo в Blynk терминал
    // terminal.print(F("rx>"));
    // terminal.print(command);
    // terminal.flush();

    doCommand(command, true);
  }

  // Обработка кнопок
  buttonHandle(PIN_POWER_BUTTON, &Button_POWER, 
    (fPower ? F(CMD_POWER_OFF) : F(CMD_POWER_ON)));                           // POWER

  buttonHandle(PIN_ACCUMULATOR_1_BUTTON, &Button_ACCUMULATOR_1, 
    (fAccumulator_1 ? F(CMD_ACCUMULATOR_1_OFF) : F(CMD_ACCUMULATOR_1_ON)));   // ACCUMULATOR 1

  buttonHandle(PIN_ACCUMULATOR_2_BUTTON, &Button_ACCUMULATOR_2, 
    (fAccumulator_2 ? F(CMD_ACCUMULATOR_2_OFF) : F(CMD_ACCUMULATOR_2_ON)));   // ACCUMULATOR 2
}


void pinsInit()
{
  // Инициализация выводов выбора устройства
  pinMode(PIN_UART_RELAY, OUTPUT);
  pinMode(PIN_UART_LED_1, OUTPUT);
  pinMode(PIN_UART_LED_2, OUTPUT);

  // Инициализация выводов вкл/откл питания
  pinMode(PIN_POWER_RELAY, OUTPUT);
  pinMode(PIN_POWER_BUTTON, INPUT_PULLUP);
  
  // Инициализация выводов подкл/откл аккумулятора #1
  pinMode(PIN_ACCUMULATOR_1_RELAY, OUTPUT);
  pinMode(PIN_ACCUMULATOR_1_BUTTON, INPUT_PULLUP);
 
  // Инициализация выводов подкл/откл аккумулятора #2
  pinMode(PIN_ACCUMULATOR_2_RELAY, OUTPUT);
  pinMode(PIN_ACCUMULATOR_2_BUTTON, INPUT_PULLUP); 
}


void buttonHandle(int pin, BUTTON* pBtn, const __FlashStringHelper *pszCmd)
{
  buttonUpdateState(pin, pBtn);
  if (pBtn->fState && pBtn->fIsFirst)
  {
    doCommand(pszCmd);
    pBtn->fIsFirst = false;
  }
}


void buttonUpdateState(int pin, BUTTON *pBtn)
{
  bool fTemp = digitalRead(pin) == 0;
  if (fTemp != pBtn->fState)
  {
    delay(20); // ms
    bool fNewState = digitalRead(pin) == 0;
    pBtn->fIsFirst = (pBtn->fState != fNewState);
    pBtn->fState = fNewState;
  }
}


void doCommand(const String &command, bool isPrint)
{
  // Status

  if (command == F(CMD_STATUS))
  {
    Serial.println(F("Current status:"));
    Serial.print(F("UART:    ")); Serial.println(curDev == DeviceIndex::DPS_1 ? F(CMD_SELECT_DPS_1) : F(CMD_SELECT_DPS_2));
    Serial.print(F("FAN:     ")); Serial.println(fPower ? F("ON") : F("OFF"));
    Serial.print(F("BLOCK 1: ")); Serial.println(fAccumulator_1 ? F("ON") : F("OFF"));
    Serial.print(F("BLOCK 2: ")); Serial.println(fAccumulator_2 ? F("ON") : F("OFF"));
  }

  // Power

  else if (command == F(CMD_POWER_ON))
  {
    fPower = true;
    digitalWrite(PIN_POWER_RELAY, HIGH);
    ledPowerSupply.on();
    if (isPrint)
    {
      delay(POWER_TOGGLE_DELAY_MS); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_ON));
    }
  }
  else if (command == F(CMD_POWER_OFF))
  {
    fPower = false;
    digitalWrite(PIN_POWER_RELAY, LOW);  
    ledPowerSupply.off();
    if (isPrint)
    {
      delay(POWER_TOGGLE_DELAY_MS); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_OFF));
    }
  }

  // UART

  else if (command == F(CMD_SELECT_DPS_2))
  {
    curDev = DeviceIndex::DPS_2;
    digitalWrite(PIN_UART_LED_1, !LED_ACTIVE);
    digitalWrite(PIN_UART_LED_2, LED_ACTIVE);
    digitalWrite(PIN_UART_RELAY, LOW);
    ledDevice_1.off();
    ledDevice_2.on();
    updateLeds();
    setMaxVoltage(data.voltage_in / 100);
    setMaxCurrent(DPS_2_CURRENT_MAX);
    if (isPrint) Serial.println(F(RESPONCE_SELECT_DEVICE_2)); 
    // set slider
    uint8_t result = modBus.readHoldingRegisters(DPS_REG_VOLTAGE_SET, 0);
    if (result == modBus.ku8MBSuccess)
    {
      uint16_t val = modBus.getResponseBuffer(0);
      Blynk.virtualWrite(PIN_SELECT_VOLTAGE, val / 100);
    }
    else
    {
      printError('B', result);
    }
  }
  else if (command == F(CMD_SELECT_DPS_1))
  {
    curDev = DeviceIndex::DPS_1;
    digitalWrite(PIN_UART_LED_1, LED_ACTIVE);
    digitalWrite(PIN_UART_LED_2, !LED_ACTIVE);
    digitalWrite(PIN_UART_RELAY, HIGH);
    ledDevice_1.on();
    ledDevice_2.off();
    updateLeds();
    setMaxVoltage(data.voltage_in / 100);
    setMaxCurrent(DPS_2_CURRENT_MAX);
    if (isPrint) Serial.println(F(RESPONCE_SELECT_DEVICE_1));
     // set slider
    uint8_t result = modBus.readHoldingRegisters(DPS_REG_CURRENT_SET, 0);
    if (result == modBus.ku8MBSuccess)
    {
      uint16_t val = modBus.getResponseBuffer(0);
      Blynk.virtualWrite(PIN_SELECT_CURRENT, val / 100);
    }
    else
    {
      printError('B', result);
    }
  }
  
  // Accumulator 1

  else if (command == F(CMD_ACCUMULATOR_1_ON))
  {
    fAccumulator_1 = true;
    digitalWrite(PIN_ACCUMULATOR_1_RELAY, HIGH);
    updateLeds();
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_1_ON)); 
  }
  else if (command == F(CMD_ACCUMULATOR_1_OFF))
  {
    fAccumulator_1 = false;
    digitalWrite(PIN_ACCUMULATOR_1_RELAY, LOW);
    updateLeds();
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_1_OFF)); 
  }
  
  // Accumulator 2

  else if (command == F(CMD_ACCUMULATOR_2_ON))
  {
    fAccumulator_2 = true;
    digitalWrite(PIN_ACCUMULATOR_2_RELAY, HIGH);
    updateLeds();
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_2_ON));
  }
  else if (command == F(CMD_ACCUMULATOR_2_OFF))
  {
    fAccumulator_2 = false;
    digitalWrite(PIN_ACCUMULATOR_2_RELAY, LOW);
    updateLeds();
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_2_OFF));
  }
}


void updateLeds()
{
  if (curDev == DeviceIndex::DPS_1)
  {
    if (fCharging_1) ledCharging.on(); else ledCharging.off();
    if (fAccumulator_1) ledAccumulator.on(); else ledAccumulator.off();
  }
  else
  {
    if (fCharging_2) ledCharging.on(); else ledCharging.off();
    if (fAccumulator_2) ledAccumulator.on(); else ledAccumulator.off();
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


void toggleCharging(bool *pfCharging)
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
//   terminal.print(F("ModBus Error "));
//   terminal.print(letter);
//   terminal.print(F("-"));
//   terminal.println(errorCode, HEX);
//   terminal.flush();
}
