# Arduino-led-matrix-video-game
This LED matrix game is based of an 
- AT2560 microcontroller (Arduino Mega)
- BTF-LIGHTING 16x16 RGB LED Matrix WS2812B - connected to pin 4 on the Arduino
- NES videogame controller - connected through I2C
- 2004 I2C LCD, used for picking the game at start, and showing the score - connected through I2C

There are two games installed and you can pick which game to play at the startup of the system. Games available are:
- Tetris
![Image of Tetris](https://github.com/ABraik-bit/Arduino-led-matrix-video-game/blob/master/Tetris.jpeg)
- Snake 
![Image of Snake](https://github.com/ABraik-bit/Arduino-led-matrix-video-game/blob/master/Snake.jpeg)


- 3D printed parts are available on the repo if you want to clone it
- The high score will be stored in the internal microcontroller EEPROM, and will be updated if broken by a new user
