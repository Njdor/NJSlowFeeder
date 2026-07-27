# NJSlowFeeder

This is a slowfeeder for feeding coffee beans into a grinder. It is designed to be 3D printed and assembled with basic mechanical and electrical tools (hex driver, soldering iron, wire strippers).

## Updates:
2025.07.07 - v1.2.1 - OOP code with additional features added by eatfish - Big Props!\
Development is actively occuring, the main discussion channel is on the Gaggiuino Discord. 

## V1.2
V1.2 introduces a microcontroller (with BLE/Wifi), PWM motor controller, and load cell compatiblity while maintaining the great battery life of V1.1.\
V1.2 CAD is stable. See Issues to request new adaptors or other features.\
V1.2.1 Code is stable, development is occuring on web-ui/ble features.
BOM at [this link](https://docs.google.com/spreadsheets/d/11zUR7dkBkgdKcGbynbE2zPVb8PsdSGaQEwi6_OAgMBY/edit?usp=sharing).
### Demo Video
[![SlowFeeder V1.2 Demo](https://img.youtube.com/vi/54PZubX1fOw/0.jpg)](https://www.youtube.com/watch?v=54PZubX1fOw)

### User Interface:
- Hold Up/Down → Gradually increase/decrease motor speed
- Single-click Up → Start motor or reset to minimum speed
- Single-click Down
   - While motor spinning → Stop motor
   - While idling, [IF Battery Monitor] → Indicate battery level (more beeps = higher battery)
- Double-click Up → Jump to max speed
- Double-click Down → Manually enter deep sleep


### Working Functions:
1. Variable speed control (hold up/down once wheel is turning)
2. On/Off control of wheel
3. Loadcell reading and setup taring (basic)
4. Auto Deep Sleep: Device will enter deep sleep automatically after 30s of motor or button idling
5. RTC memory support: Saves last-used motor speed across sleep cycles

### Planned Functions:
- [ ] WebUI for parameter control
- [ ] PoopCode :tm: - Precise single dosing of beans (for filling vials or grinding)

### Getting Started with V1.2 Lego Build
1. Review V1.2 BOM
2. Decide what optional features you want to integrate
   - Load Cell (stop on weight. No power budget impact)
   - Buzzer (Louder tones instead of motor driven tones, minor power budget impact)
   - Battery Voltage Monitor (Voltage divider method, minor power budget impact)
4. Open CAD_Models folder and determine CAD files needed to build your slow feeder
   - Xiao or Supermini body
   - Door
   - Grinder adaptor (in folder structure per grinder)
   - Feeder wheel (8 hole default, other hole counts in folder)
   - Load Cell Ring (if adding load cell)
5. Use 3DPrinting orientation image to arrange your prints as shown
6. Use Xiao Wiring Diagram to determine how to wire your slowfeeder
7. Assemble everything in place with screws. (Assembly video coming soon)

### Getting Started with V1.2 Code
1. Install VSCode
2. Clone this repo
3. Open the NJSlowfeeder.code-workspace file
   - This should install all required dependencies
4. Open folder in Platformio extension (if not already selected)
5. Set build flags in platformio.ini (if your configuration is not already included (depends on what extra features you've added.
   - Valid flags:
      - CALIBRATION_FACTOR
      - HAS_LOADCELL
      - HAS_BATTERYMONITOR
      - HAS_BUZZER
7. Choose the MCU you have in the Platformio Project Tasks section, plug MCU into computer, click upload (if MCU has already been flashed and is in deep-sleep, wake it by clicking up button before plugging in)
   - You will need to manually change pin definition if using SuperMini. SuperMini pin define will be added to include/board.h when time allows 
   - If this fails, hold the boot button and then click the reset button while the MCU is powered (battery or usb)
8. Done

## V1.1 
V1.1 uses a fully analog approach to slowfeeding and does not include a microcontroller.\
V1.1 is unlikely to receive further updates. An analog V1.2 can be made with some tweaks to the BOM.\
Speed is set using a regulator of user choice (good options listed in BOM) and is not easily adjustable on the fly. \
See the V1.1 folder in CAD_Models for more details.\
BOM at [this link](https://docs.google.com/spreadsheets/d/11zUR7dkBkgdKcGbynbE2zPVb8PsdSGaQEwi6_OAgMBY/edit?usp=sharing).
