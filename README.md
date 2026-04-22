Laser Tag Game
Creates local host server
Creates html at ip
Communication between server and client via TCP
  Process hits
  Parse URL
Generates encoded IR packets to communicate between players
  Start bit low, Start bit high, 8 bit player data, even parity bit, stop bit high
Receivers demodulate IR signal and communicates information to host
Trigger button triggers interrupt flags to fire IR
Receivers trigger interrupt flag on falling edge of IR reception
Audio feedback played through speaker

Espressif ESP32c6 - Host Protocols, creates softAP, webserver for http, and device server for tcp.

Espressif ESP32c6 x3 - Player Protocols, creates wifi station and joins host network. Communicates with server via tcp.

Adafruit IR Remote Receiver x6 - Falling edge IR signal triggers interrupt flags to prepare receiver to decode IR packet.
Packet is then communicated to device server.

Adafruit High Power IR LED Emitter x3 - Trigger flags prompt emitter to send IR packet encoded with PlayerID.

Button - Triggers interrupt on rising edge to set Trigger flag high to prompt Emitter fire.

Speaker - Plays audio.

Problems encountered - alot.

IR timing change due to overhead.

Watchdog timer button instead of trigger button.

Website url refresh bug.

WiFi pain. ESP32C6 pain.

Similar - 

Different - 

Improvements - 


