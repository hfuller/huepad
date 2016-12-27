# huepad

This is a nine-button keypad for controlling your Hue lights. It controls the first six groups of lights on your bridge, and it supports toggling, dimming, and the "color loop" special effect.

![Assembled huepad controller](https://i.imgur.com/0kjx2L2.jpg)

## Assemble

1. Acquire these things:
    * [WeMos D1](https://www.wemos.cc/product/d1.html)
    * [ITEAD IBridge Lite](https://www.itead.cc/ibridge-lite.html)
    * micro-USB cable and power source (or, alternatively, a 9-24V power supply for the DC input connector)
2. Connect the IBridge Lite to the D1, aligning the gap in the pins with the gap between the sockets.
3. Pop off the three blue buttons that are farthest from the micro-USB connector. This will make the dimmer buttons distinct from the toggle buttons.
4. [Download the Arduino IDE](https://www.arduino.cc/en/Main/Software).
5. [Do this](https://github.com/esp8266/Arduino/blob/master/README.md#using-git-version). (As of this writing, you really do need the git HEAD of that repo to build this one. Sorry.)
6. From your terminal:
    * `cd ~/Arduino`
    * `git clone https://github.com/hfuller/huepad`
7. Open the huepad.ino file in Arduino, select the D1 from the board configuration menu, and hit Ctrl+U to upload to the board.

## Initial setup

1. Boot up the device.
2. Use your phone to connect to the wireless network that looks like "huepad Setup".
3. Follow the instructions to connect the device to your wireless network.
4. Wait 30 seconds.
5. Press the button on your Hue bridge.
6. Wait 30 seconds.

## Use

There are a few things you can do.

* Press the first through sixth buttons on the controller to toggle the first through sixth groups of lights.
* While holding one of those buttons, you can:
    * Hold the bottom left button to dim those lights.
    * Hold the bottom right button to brighten those lights.
    * Press the bottom center button to activate the color wheel effect on those lights. (The lights have to support color for this to make any sense.)
* I know I just said you have to be *holding* one of the first six buttons, but that's not quite true. You can press the dimmer buttons at any time to dim or brighten the lights you most recently controlled from this controller, too.

Enjoy!
