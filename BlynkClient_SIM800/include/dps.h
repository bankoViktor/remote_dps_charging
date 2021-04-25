#ifndef DPS__H
#define DPS__H

// DPS5020 CNC Power Communication Protocol V1.2
// https://cloud.kyme32.ro/ftp_backup/DPS5020%20PC%20Software(2017.11.04)/DPS5020%20CNC%20Communication%20%20Protocol%20V1.2.pdf

// #1  DPS 5020  0-50V  0-20A
// #2  DPS 5015  0-50V  0-15A

#define DPS_REG_VOLTAGE_SET         0x0000  // U-SET        - RW    - Voltage setting 
#define DPS_REG_CURRENT_SET         0x0001  // I-SET        - RW    - Current setting
#define DPS_REG_VOLTAGE_OUT         0x0002  // UOUT         - R     - Output voltage display value
#define DPS_REG_CURRENT_OUT         0x0003  // IOUT         - R     - Output current display value
#define DPS_REG_POWER_OUT           0x0004  // POWER        - R     - Output power display value
#define DPS_REG_VOLTAGE_IN          0x0005  // UIN          - R     - Input voltage display value
#define DPS_REG_LOCK                0x0006  // LOCK         - RW    - Key lock
#define DPS_REG_PROTECT             0x0007  // PROTECT      - R     - Protection status
//  0 - good running 
//  1 - represents OVP (Over-voltage protection)
//  2 - represents OCP (Over-current protection)
//  3 - represents OPP (Over-power protection)
#define DPS_REG_CV_CC               0x0008  // CV/CC        - R     - Constant Voltage / Constant Current status 
#define DPS_REG_ONOFF               0x0009  // ONOFF        - RW    - Switch output state
#define DPS_REG_B_LED               0x000A  // B_LED        - RW    - Backlight brightness level
#define DPS_REG_MODEL               0x000B  // MODEL        - R     - Product model 
#define DPS_REG_VERSON              0x000C  // VERSON       - R     - Firmware Version

#define DPS_REG_EXTRACT_M           0x0023  // EXTRACT_M    - RW    - Shortcut to bring up the required data set 

#define DPS_REG_USET                0x0050  // U-SET        - RW    - Voltage settings
#define DPS_REG_ISET                0x0051  // I-SET        - RW    - Current setting
#define DPS_REG_SOVP                0x0052  // S-OVP        - RW    - Over-voltage protection value
#define DPS_REG_SOCP                0x0053  // S-OCP        - RW    - Over-current protection value
#define DPS_REG_SOPP                0x0054  // S-OPP        - RW    - Over power protection
#define DPS_REG_BLED                0x0055  // B-LED        - RW    - Backlight brightness levels
#define DPS_REG_MPRE                0x0056  // M-PRE        - RW    - Memory Preset Number
#define DPS_REG_SINI                0x0057  // S-INI        - RW    - Power output switch


#define DPS_1_CURRENT_MAX           20      // A  DPS 5020
#define DPS_2_CURRENT_MAX           15      // A  DPS 5015

#endif
