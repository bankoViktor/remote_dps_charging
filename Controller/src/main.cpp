/*
 * Исполнительный механизм
 *
 * Версия:    0.1
 * Дата:      22.04.2021
 * Автор:     bankviktor14@gmail.com
 */


#include <Arduino.h>
#include "../../Shared/include/commands.h"


/* Private Defines ------------------------------------------------------------ */

#define UART_SPEED                        9600                 // скорость UART (в бодах)
#define LED_ACTIVE                        LOW
#define POWER_TOGGLE_DELAY_MS             1000                 // ms
// Select Device   

#define PIN_UART_LED_1                    12                   // номер вывода светодиода UART 1
#define PIN_UART_LED_2                    11                   // номер вывода светодиода UART 2
#define PIN_UART_RELAY                    13                   // номер вывода реле переключения устройства UART

// Power   

#define PIN_POWER_LED                     9                    // номер вывода светодиода POWER
#define PIN_POWER_RELAY                   10                   // номер вывода реле POWER
#define PIN_POWER_BUTTON                  4                    // номер вывода кнопки POWER

// Accumulator 1   

#define PIN_ACCUMULATOR_1_LED             7                    // номер вывода светодиода ACCUMULATOR 1
#define PIN_ACCUMULATOR_1_RELAY           8                    // номер вывода реле ACCUMULATOR 1
#define PIN_ACCUMULATOR_1_BUTTON          3                    // номер вывода кнопки ACCUMULATOR 1

// Accumulator 2   

#define PIN_ACCUMULATOR_2_LED             5                    // номер вывода светодиода ACCUMULATOR 2
#define PIN_ACCUMULATOR_2_RELAY           6                    // номер вывода реле ACCUMULATOR 2
#define PIN_ACCUMULATOR_2_BUTTON          2                    // номер вывода кнопки ACCUMULATOR 2


/* Private Types -------------------------------------------------------------- */

struct BUTTON
{
  bool fState = false;
  bool fIsFirst = false;
};


/* Private Variables ---------------------------------------------------------- */

BUTTON Button_POWER;
BUTTON Button_ACCUMULATOR_1;
BUTTON Button_ACCUMULATOR_2;

bool fUART;           // 0 - UART #1; 1 - UART #2
bool fPower;             
bool fAccumulator_1;     
bool fAccumulator_2;     


/* Private Function Declarations ---------------------------------------------- */

void DoCommand(const String command, bool isPrint = false);
void PrintStatus();
void Button_Handle(int pin, BUTTON* pBtn, const __FlashStringHelper * pszCmd);
void Button_UpdateState(int pin, BUTTON* pBtn);


/* Private Function Definitions ----------------------------------------------- */

void setup()
{
  Serial.begin(UART_SPEED);

  // Инициализация выводов выбора устройства
  pinMode(PIN_UART_RELAY, OUTPUT);
  pinMode(PIN_UART_LED_1, OUTPUT);
  pinMode(PIN_UART_LED_2, OUTPUT);
  DoCommand(F(CMD_SELECT_DPS_1));

  // Инициализация выводов вкл/откл питания
  pinMode(PIN_POWER_RELAY, OUTPUT);
  pinMode(PIN_POWER_LED, OUTPUT);
  pinMode(PIN_POWER_BUTTON, INPUT_PULLUP);
  DoCommand(F(CMD_POWER_OFF));
  
  // Инициализация выводов подкл/откл аккумулятора #1
  pinMode(PIN_ACCUMULATOR_1_RELAY, OUTPUT);
  pinMode(PIN_ACCUMULATOR_1_LED, OUTPUT);
  pinMode(PIN_ACCUMULATOR_1_BUTTON, INPUT_PULLUP);
  DoCommand(F(CMD_ACCUMULATOR_1_OFF));
 
  // Инициализация выводов подкл/откл аккумулятора #2
  pinMode(PIN_ACCUMULATOR_2_RELAY, OUTPUT);
  pinMode(PIN_ACCUMULATOR_2_LED, OUTPUT);
  pinMode(PIN_ACCUMULATOR_2_BUTTON, INPUT_PULLUP); 
  DoCommand(F(CMD_ACCUMULATOR_2_OFF));
}


void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');

    command.trim();

    if (command[command.length() - 1] == '\r')
    {
      command.remove(command.length() - 1);
    }

    if (command == F(CMD_STATUS))
    {
      Serial.println(F("\r\nCurrent status:"));
      PrintStatus();
      Serial.println();
    }
    else
    {
      DoCommand(command, true);
    }
  }

  // Обработка кнопок режимов
  Button_Handle(PIN_POWER_BUTTON, &Button_POWER, fPower ? F(CMD_POWER_OFF) : F(CMD_POWER_ON));                     // POWER
  Button_Handle(PIN_ACCUMULATOR_1_BUTTON, &Button_ACCUMULATOR_1, fAccumulator_1 ? F(CMD_ACCUMULATOR_1_OFF) : F(CMD_ACCUMULATOR_1_ON));   // ACCUMULATOR 1
  Button_Handle(PIN_ACCUMULATOR_2_BUTTON, &Button_ACCUMULATOR_2, fAccumulator_2 ? F(CMD_ACCUMULATOR_2_OFF) : F(CMD_ACCUMULATOR_2_ON));   // ACCUMULATOR 2
}


void Button_Handle(int pin, BUTTON* pBtn, const __FlashStringHelper * pszCmd)
{
  Button_UpdateState(pin, pBtn);
  if (pBtn->fState && pBtn->fIsFirst)
  {
    DoCommand(pszCmd);
    pBtn->fIsFirst = false;
  }
}


void Button_UpdateState(int pin, BUTTON* pBtn)
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


void DoCommand(const String command, bool isPrint)
{
  // Power
  if (command == F(CMD_POWER_ON))
  {
    fPower = true;
    digitalWrite(PIN_POWER_LED, LED_ACTIVE);
    digitalWrite(PIN_POWER_RELAY, HIGH);
    if (isPrint)
    {
      delay(POWER_TOGGLE_DELAY_MS); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_ON));
    }
  }
  else if (command == F(CMD_POWER_OFF))
  {
    fPower = false;
    digitalWrite(PIN_POWER_LED, !LED_ACTIVE);
    digitalWrite(PIN_POWER_RELAY, LOW);  
    if (isPrint)
    {
      delay(POWER_TOGGLE_DELAY_MS); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_OFF));
    }
  }

  // UART
  else if (command == F(CMD_SELECT_DPS_2))
  {
    fUART = true;
    digitalWrite(PIN_UART_LED_1, !LED_ACTIVE);
    digitalWrite(PIN_UART_LED_2, LED_ACTIVE);
    digitalWrite(PIN_UART_RELAY, LOW);
    if (isPrint) Serial.println(F(RESPONCE_SELECT_DEVICE_2)); 
  }
  else if (command == F(CMD_SELECT_DPS_1))
  {
    fUART = false;
    digitalWrite(PIN_UART_LED_1, LED_ACTIVE);
    digitalWrite(PIN_UART_LED_2, !LED_ACTIVE);
    digitalWrite(PIN_UART_RELAY, HIGH);
    if (isPrint) Serial.println(F(RESPONCE_SELECT_DEVICE_1));
  }
  
  // Accumulator 1
  else if (command == F(CMD_ACCUMULATOR_1_ON))
  {
    digitalWrite(PIN_ACCUMULATOR_1_LED, LED_ACTIVE);
    digitalWrite(PIN_ACCUMULATOR_1_RELAY, HIGH);
    fAccumulator_1 = true;
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_1_ON)); 
  }
  else if (command == F(CMD_ACCUMULATOR_1_OFF))
  {
    digitalWrite(PIN_ACCUMULATOR_1_LED, !LED_ACTIVE);
    digitalWrite(PIN_ACCUMULATOR_1_RELAY, LOW);
    fAccumulator_1 = false;
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_1_OFF)); 
  }
  
  // Accumulator 2
  else if (command == F(CMD_ACCUMULATOR_2_ON))
  {
    digitalWrite(PIN_ACCUMULATOR_2_LED, LED_ACTIVE);
    digitalWrite(PIN_ACCUMULATOR_2_RELAY, HIGH);
    fAccumulator_2 = true;
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_2_ON));
  }
  else if (command == F(CMD_ACCUMULATOR_2_OFF))
  {
    digitalWrite(PIN_ACCUMULATOR_2_LED, !LED_ACTIVE);
    digitalWrite(PIN_ACCUMULATOR_2_RELAY, LOW);
    fAccumulator_2 = false;
    if (isPrint) Serial.println(F(RESPONCE_ACCUMULATOR_2_OFF));
  }
}


void PrintStatus()
{
  Serial.print(F("UART:    ")); Serial.println(fUART ? F(CMD_SELECT_DPS_2) : F(CMD_SELECT_DPS_1));
  Serial.print(F("FAN:     ")); Serial.println(fPower ? F("ON") : F("OFF"));
  Serial.print(F("BLOCK 1: ")); Serial.println(fAccumulator_1 ? F("ON") : F("OFF"));
  Serial.print(F("BLOCK 2: ")); Serial.println(fAccumulator_2 ? F("ON") : F("OFF"));
}
