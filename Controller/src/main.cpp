/*
 * Исполнительный механизм
 *
 * Версия:    0.1
 * Дата:      22.04.2021
 * Автор:     bankviktor14@gmail.com
 */


#include <Arduino.h>
#include "../../commands.h"


#define UART_SPEED                  9600                // скорость UART (в бодах)
#define KEY_WORD_STATUS             CMD_STATUS 

// --------------------------------------------------------------------------------

#define KEY_WORD_0_ON               CMD_SELECT_DPS_2    // ключевое слово для включение режима
#define KEY_WORD_0_OFF              CMD_SELECT_DPS_1    // ключевое слово для выключения режима
#define LED1_0_PIN                  11                  // номер вывода светодиода
#define LED2_0_PIN                  12                  // номер вывода светодиода
#define RELAY_0_PIN                 13                  // номер вывода реле
//#define BUTTON_0_PIN                                    // номер вывода кнопки

// --------------------------------------------------------------------------------

#define KEY_WORD_1_ON               CMD_POWER_ON        // ключевое слово для включение режима
#define KEY_WORD_1_OFF              CMD_POWER_OFF       // ключевое слово для выключения режима
#define LED_1_PIN                   9                   // номер вывода светодиода
#define RELAY_1_PIN                 10                  // номер вывода реле
#define BUTTON_1_PIN                4                   // номер вывода кнопки

// --------------------------------------------------------------------------------

#define KEY_WORD_2_ON               CMD_CHARGING_1_ON   // ключевое слово для включение режима
#define KEY_WORD_2_OFF              CMD_CHARGING_1_OFF  // ключевое слово для выключения режима
#define LED_2_PIN                   7                   // номер вывода светодиода
#define RELAY_2_PIN                 8                   // номер вывода реле
#define BUTTON_2_PIN                3                   // номер вывода кнопки

// --------------------------------------------------------------------------------

#define KEY_WORD_3_ON               CMD_CHARGING_2_ON   // ключевое слово для включение режима
#define KEY_WORD_3_OFF              CMD_CHARGING_2_OFF  // ключевое слово для выключения режима
#define LED_3_PIN                   5                   // номер вывода светодиода
#define RELAY_3_PIN                 6                   // номер вывода реле
#define BUTTON_3_PIN                2                   // номер вывода кнопки

/* Private Types -------------------------------------------------------------- */

struct BUTTON
{
  bool fState = false;
  bool fIsFirst = false;
};


/* Private Variables ---------------------------------------------------------- */

//BUTTON Button_0;
BUTTON Button_1;
BUTTON Button_2;
BUTTON Button_3;

bool fMode_UART;      // UART
bool fMode_FAN;       // FAN
bool fMode_BLOCK_1;   // BLOCK 1
bool fMode_BLOCK_2;   // BLOCK 2


/* Private Function Declarations ---------------------------------------------- */

void DoCommand(String command, bool isPrint = false);
void PrintStatus();
void Button_Handle(int pin, BUTTON* pBtn, const char *pszCmd);
void Button_UpdateState(int pin, BUTTON* pBtn);


/* Private Function Definitions ----------------------------------------------- */

void setup()
{
  Serial.begin(UART_SPEED);

  // Инициализация выводов для режима №0
  pinMode(RELAY_0_PIN, OUTPUT);
  pinMode(LED1_0_PIN, OUTPUT);
  pinMode(LED2_0_PIN, OUTPUT);
  //pinMode(BUTTON_0_PIN, INPUT_PULLUP); // если кнопка не нужна, закоментируй эту строку, и соответствующую строку в функции loop
  DoCommand(F(KEY_WORD_0_OFF));

  // Инициализация выводов для режима №1
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);  // см выше
  DoCommand(F(KEY_WORD_1_OFF));
  
  // Инициализация выводов для режима №2
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);  // см выше
  DoCommand(F(KEY_WORD_2_OFF));
 
  // Инициализация выводов для режима №3
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(LED_3_PIN, OUTPUT);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);  // см выше
  DoCommand(F(KEY_WORD_3_OFF));
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

    if (command == F(KEY_WORD_STATUS))
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
  // !!!!! ЗАКОМЕНТИРУЙ строку если не нужна кнопка в соответствующем режиме
  //Button_Handle(BUTTON_0_PIN, &Button_0, fMode_0 ? KEY_WORD_0_OFF : KEY_WORD_0_ON);       // MODE 0
  Button_Handle(BUTTON_1_PIN, &Button_1, fMode_UART ? KEY_WORD_1_OFF : KEY_WORD_1_ON);      // MODE 1
  Button_Handle(BUTTON_2_PIN, &Button_2, fMode_BLOCK_1 ? KEY_WORD_2_OFF : KEY_WORD_2_ON);   // MODE 2
  Button_Handle(BUTTON_3_PIN, &Button_3, fMode_BLOCK_2 ? KEY_WORD_3_OFF : KEY_WORD_3_ON);   // MODE 3
}


// Обработка кнопки
void Button_Handle(int pin, BUTTON* pBtn, const char *pszCmd)
{
  Button_UpdateState(pin, pBtn);
  if (pBtn->fState && pBtn->fIsFirst)
  {
    DoCommand(pszCmd);
    pBtn->fIsFirst = false;
  }
}


// Обновляет состояние кнопки
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


// Принудительное выполнение команды
void DoCommand(String command, bool isPrint)
{
  // Режим 0
  if (command == F(KEY_WORD_0_ON))
  {
    digitalWrite(LED1_0_PIN, LOW);
    digitalWrite(RELAY_0_PIN, LOW);
    digitalWrite(LED2_0_PIN, HIGH);
    fMode_UART = true;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_SELECT_DEVICE_2));
    }
  }
  else if (command == F(KEY_WORD_0_OFF))
  {
    digitalWrite(LED1_0_PIN, HIGH);
    digitalWrite(LED2_0_PIN, LOW);
    digitalWrite(RELAY_0_PIN, HIGH);
    fMode_UART = false;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_SELECT_DEVICE_1));
    }
  }
  
  // Режим 1
  else if (command == F(KEY_WORD_1_ON))
  {
    digitalWrite(LED_1_PIN, LOW);
    digitalWrite(RELAY_1_PIN, HIGH);
    fMode_FAN = true;
    if (isPrint)
    {
      delay(1000); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_ON));
    }
  }
  else if (command == F(KEY_WORD_1_OFF))
  {
    digitalWrite(LED_1_PIN, HIGH);
    digitalWrite(RELAY_1_PIN, LOW);
    fMode_FAN = false;
    if (isPrint)
    {
      delay(1000); // пропускаем помехи
      Serial.println(F(RESPONCE_POWER_OFF));
    }
  }
  
  // Режим 2
  else if (command == F(KEY_WORD_2_ON))
  {
    digitalWrite(LED_2_PIN, LOW);
    digitalWrite(RELAY_2_PIN, HIGH);
    fMode_BLOCK_1 = true;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_BLOCK_1_ON));
    }
  }
  else if (command == F(KEY_WORD_2_OFF))
  {
    digitalWrite(LED_2_PIN, HIGH);
    digitalWrite(RELAY_2_PIN, LOW);
    fMode_BLOCK_1 = false;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_BLOCK_1_OFF));
    }
  }
  
  // Режим 3
  else if (command == F(KEY_WORD_3_ON))
  {
    digitalWrite(LED_3_PIN, LOW);
    digitalWrite(RELAY_3_PIN, HIGH);
    fMode_BLOCK_2 = true;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_BLOCK_2_ON));
    }
  }
  else if (command == F(KEY_WORD_3_OFF))
  {
    digitalWrite(LED_3_PIN, HIGH);
    digitalWrite(RELAY_3_PIN, LOW);
    fMode_BLOCK_2 = false;
    if (isPrint)
    {
      Serial.println(F(RESPONCE_BLOCK_2_OFF));
    }
  }
}


// Вывод состояния режимов
void PrintStatus()
{
  Serial.print(F("UART:    ")); Serial.println(fMode_UART ? KEY_WORD_0_ON : KEY_WORD_0_OFF);
  Serial.print(F("FAN:     ")); Serial.println(fMode_FAN ? F("ON") : F("OFF")); 
  Serial.print(F("BLOCK 1: ")); Serial.println(fMode_BLOCK_1 ? F("ON") : F("OFF")); 
  Serial.print(F("BLOCK 2: ")); Serial.println(fMode_BLOCK_2 ? F("ON") : F("OFF")); 
}
