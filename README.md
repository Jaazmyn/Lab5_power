# Lab5_power
**Fundamental Strategy:**
- Use deep sleep mode when no movement is detected.
- Reduce WiFi usage by sending data only when necessary.
- Adjust ultrasonic sensor polling frequency to reduce power consumption.

**Power Saving Policy**
- Object Detected (Within 50cm): The device remains active and transmits distance data to Firebase every 2 seconds. If movement is detected (distance change > 2cm), the system resets the inactivity timer.
- Object Moves Out of Range (>50cm): If the detected object moves beyond 50 cm, the device enters a short sleep (30 seconds) to conserve power.
- No Movement for 30 Seconds: If no movement is detected within 30 seconds, the system assumes inactivity and enters a short sleep (30 seconds).
- Deep Sleep Mode: During sleep, the WiFi and ultrasonic sensor are turned off to minimize power consumption.

