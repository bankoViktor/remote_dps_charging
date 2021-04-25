#ifndef PINS__H
#define PINS__H

// Select Device   

#define PIN_UART_LED_1                      12    // номер вывода светодиода UART 1
#define PIN_UART_LED_2                      11    // номер вывода светодиода UART 2
#define PIN_UART_RELAY                      13    // номер вывода реле переключения устройства UART

// Power    

#define PIN_POWER_RELAY                     10    // номер вывода реле POWER
#define PIN_POWER_BUTTON                    4     // номер вывода кнопки POWER

// Accumulator 1    

#define PIN_ACCUMULATOR_1_RELAY             8     // номер вывода реле ACCUMULATOR 1
#define PIN_ACCUMULATOR_1_BUTTON            7     // номер вывода кнопки ACCUMULATOR 1

// Accumulator 2    

#define PIN_ACCUMULATOR_2_RELAY             6     // номер вывода реле ACCUMULATOR 2
#define PIN_ACCUMULATOR_2_BUTTON            5     // номер вывода кнопки ACCUMULATOR 2

// Soft Serial SIM800   

#define PIN_SIM800_RX                       2
#define PIN_SIM800_TX                       3

// Blynk Pins

#define PIN_POWER_SUPPLY_LED                V0    // индикатор питания
#define PIN_POWER_SUPPLY                    V1    // переключение питания
#define PIN_SELECT_DEVICE                   V2    // выбор устройства
#define PIN_VOLTAGE_IN                      V3    // напряжение входное
#define PIN_VOLTAGE_OUT_OR_ACC              V4    // напряжение установка/выход/аккум
#define PIN_POWER                           V5    // мощность
#define PIN_CURRENT_OUT                     V6    // ток выходной
#define PIN_SELECT_VOLTAGE                  V7    // установка напряжения выходного
#define PIN_SELECT_CURRENT                  V8    // установка ток выходного
#define PIN_CHARGING_LED                    V9    // индикатор включения зарядки устройства 
#define PIN_CHARGING                        V10   // включение зарядки устройства
#define PIN_ACCUMULATOR_LED                 V11   // индикатор подключения аккумулятора к устройству
#define PIN_ACCUMULATOR                     V12   // подключение аккумулятора к устройству
#define PIN_DEVICE_1_LED                    V13   // индикатор устройства #1
#define PIN_DEVICE_2_LED                    V14   // индикатор устройства #2
#define PIN_TERMINAL                        V20   // Лог-терминал

#endif // PINS__H
