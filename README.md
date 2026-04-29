# Laser Tag

## Purpose
The purpose of the Laser Tag project is to create an entirely custom portable battery-powered laser-tag game, where multiple players can wirelessly interface with a server that contains game logic, and can fire "lasers" (invisible signals) at each other to play the game.

---

## Components

### Adafruit 5639 IR LED Emitter
38 kHz IR emitter used to transmit encoded data packets between players. Activated by trigger input.

### Adafruit 5939 IR Remote Receiver
38 kHz IR receiver used to demodulate incoming data packets from other players. Output routed as an external interrupt.

### SPST Button
Trigger input used to initiate encoding and transmission of the player's ID.

### ESP32-C6 Development Kit

**Server Module**
Hosts a SoftAP, HTTP server, and TCP server, managing all network communication:
- Accepts incoming player messages and distributes game state and player information via TCP.
- Handles HTTP requests to update and deliver information to the web interface.

**Player Module**
Communicates with the server and manages all core functions:
- Encodes and transmits player's ID as an IR data frame via the emitter on button interrupt.
- Decodes incoming IR data from the receiver interrupt.
- Drives audio feedback to the speaker via PWM.

### PWM Speaker
Audio feedback device used to signal the player of game events, including:
- Power on
- Connected
- Disconnected
- Player Hit
- Low Health
- Player Dead
- Fire (Enabled)
- Fire (Disabled)
- Win
- Lose

### Battery Housing / Batteries / SPDT Switch
Three 1.5V AA batteries wired in series, supplying 4.5V to the ESP32-C6. Housed in a 3×AA battery casing with an SPDT switch for power control.

### Vest & Velcro
T-shirt worn by the player to serve as a mounting surface for the game modules. Velcro used to attach and secure each module to the vest.

> Refer to the [project slideshow](LaserTag.pptx) for a fully labelled component diagram.
---

## System Overview
Each player is equipped with two receivers (one on the front-mounted player module and one on the back-mounted standalone module), one handheld emitter, and a battery pack. Upon powering on, each player module will automatically connect to the server. To select a game mode, configure game settings, and start the game, connect to the server's Wi-Fi and navigate to the game website at `192.168.4.1`.

---

## Game Logic
There are two game modes for Laser Tag: Free-For-All and Duels.

### Free-For-All
Players compete for the duration set by the website timer. Starting from 0, each hit landed adds 100 points and each hit received deducts 50. The player with the most points when the timer expires wins. Equal scores result in a draw.

### Duels
Players compete until one player remains (or none). Each player starts with 5 HP, which decrements by 1 for every hit received. The last player with nonzero HP wins. If all remaining players reach 0 HP simultaneously, there is no winner.

---

## Encountered Issues

### IR Timing
During testing, we noticed that transmitted bits were not being received correctly. After hardware debugging and analyzing the signals with an oscilloscope, we found that the receiver was detecting everything as expected, just with the incorrect timing. We had not accounted for the overhead between bit transmissions, meaning the firmware was sending bits every ~500 µs when they were being transmitted every ~700 µs. Once identified, we measured the overhead and adjusted the timing accordingly in both transmission and reception, resolving the issue. This overhead will be re-measured and recalibrated after every firmware change.

### Watchdog Timer
When designing the trigger logic, the IR write function was initially placed inside the interrupt handler. The button worked correctly until this logic was introduced, at which point the watchdog timer began tripping. We determined that the write function was taking too long within the interrupt, causing the watchdog to trigger before the write could complete. This was resolved by having the interrupt simply set a flag, which is then polled in the main loop. This immediately fixed the issue.

### Website Refresh Bug
HTTP requests were completed and parsed via the website URL, which caused page refreshes to retain the previous URL extension and repeat the last user action. This was resolved by implementing a helper function that clears the URL after each action and redirects back to the root IP address.

### Dev. Board Challenges
The UM TinyS3 was initially chosen as the MCU for both the server and player modules due to its Wi-Fi capabilities, strong performance, and compact form factor. In practice, however, it struggled to reliably host the servers and maintain player connections in areas of significant interference. Despite having a better antenna than the ESP32-C6 DevKit, its Wi-Fi 4 protocol appeared to be the bottleneck, prompting the switch to the ESP32-C6. The ESP32-C6 was able to reliably host the server and maintain player connections, though the platform change introduced IR timing issues that required recalibration before the system functioned correctly.

---

## Differences & Similarities to Commercial Laser Tag
We wanted our version of laser tag to be as close to the real thing as possible, and the core requirements to get there are simple. At its core, players get to shoot lasers at each other. Most implementations track different statistics (who was hit the most, who was the most accurate, who was the most trigger-happy, etc.). However, what makes ours stand out is not just that we track who hits who, but we do it by encoding the player ID directly into the IR transmission. On top of that, the immediate audio feedback keeps players informed on if they can fire, when they have fired, and when they've been hit.

Because our server is entirely portable, you can play anywhere you like. You can even adjust game settings from any internet-capable device by connecting directly to our server and using the hosted website. This works at any time the server is running, with no limit on how many devices can connect at once.

Our solution is also significantly cheaper than any commercial laser tag system you would find online.

---

## Future Improvements
The following improvements are planned for Laser Tag v2:

| Improvement | Description |
|---|---|
| **Custom Module Housing** | 3D-printed enclosures for the server and each player sub-module. |
| **Proper Vests** | Sewn-on fastening system for a cleaner, more permanent solution. |
| **Rechargeable Battery Power** | Lithium-Ion batteries to replace the current AA battery packs on both modules. |
| **Migration to STM** | Transition away from the ESP32 to the more industry-standard STM platform. |
| **Multi-Threading** | Enable simultaneous IR transmission and reception on player modules via multi-threading. |
| **Antenna** | Add antennas to the server and player modules for improved connection, reliability, and range. |
| **PCBs** | Design and fabricate custom boards for each sub-module for better connectivity and a smaller form factor. |
| **Live Website Updates** | Rewrite the front-end in JavaScript to push live updates, eliminating the need for a page refresh. |

