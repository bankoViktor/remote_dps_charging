#ifndef COMMANDS__H
#define COMMANDS__H

// Command Requests

#define CMD_STATUS                          "status"
#define CMD_POWER_ON                        "fanon"
#define CMD_POWER_OFF                       "fanoff"
#define CMD_SELECT_DPS_1                    "dps1"          // DPS 5020
#define CMD_SELECT_DPS_2                    "dps2"          // DPS 5015
#define CMD_ACCUMULATOR_1_ON                "block1on"
#define CMD_ACCUMULATOR_1_OFF               "block1off"
#define CMD_ACCUMULATOR_2_ON                "block2on"
#define CMD_ACCUMULATOR_2_OFF               "block2off"

// Command Responces        

#define RESPONCE_POWER_ON                   "FAN:ON"
#define RESPONCE_POWER_OFF                  "FAN:OFF"
#define RESPONCE_SELECT_DEVICE_1            "UART:DPS1"
#define RESPONCE_SELECT_DEVICE_2            "UART:DPS2"
#define RESPONCE_ACCUMULATOR_1_ON           "BLOCK_1:ON"
#define RESPONCE_ACCUMULATOR_1_OFF          "BLOCK_1:OFF"
#define RESPONCE_ACCUMULATOR_2_ON           "BLOCK_2:ON"
#define RESPONCE_ACCUMULATOR_2_OFF          "BLOCK_2:OFF"

#endif // COMMANDS__H
