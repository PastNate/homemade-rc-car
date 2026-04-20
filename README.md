# homemade-rc-car
Creating an RC Car from consumer electronic parts.

The first step in this project was designing a circuit to ascertain an idea of what components and circuitry I would need.
*Designed as a class project for UW-Madison ECE210

<img width="2597" height="1192" alt="Schematic_ECE210-Final-Project-RC-Car-v4_2026-04-19" src="https://github.com/user-attachments/assets/25298bf2-ef99-4be4-8fbb-8ea41083ab7d" />

This ended up being my final circuit design but I ended up updating my design twice throughout the process of designing my project.

  In my design I utilize an ESP32 dev kit to wirelessly control the RC Car. The devkit provides a lot of ease of access in designing the code and flexibility in its utilization.
However, the dev kit does also provide some challenges; due to the nature of the product, the datasheet and materials which were readily avaliable were for the ESP32 chip itself. 
There were very few resources for the dev board itself, which is why I had intially gotten a regulator to step down the 6V battery to the 5V/3V3 the ESP32 utilized.
Now that I have done a project like this I realize that the difference in voltage was because a regulator came with the board itself, and the 5V pin had enough tolerance for 6V.
  One possible upgrade I would consider for the future is switching out the ESP32 for a device which can handle WiFi and analog I/O. So that I could control my motor speed via a PWM circuit.

  The next component I have are my two motor controllers, the L298N. Overall a good pick, they have a good deal of control and the version I bought was easy to integrate due to its connectors.
My only note is I would want a controller which has built in PWM circuit I could control with an analog signal.

  Then for my motors, I chose the 3802 TT motors by Adafruit. They are simple but reliable, they have a high gear ratio which was nice because I was only running at 6V.
However, the provided datasheet and desgin do have some flaws. When preparing a case for my motors I ran into an issue where the motor shaft was slightly exposed and my case was too tight, preventing it from rotating.
Additionally, there is a tab at the end of the motor that was not included in the datasheet. Although minor issues it did require a redesign and reprint. (I had to print before I got my parts due to shipping delays)

  Finally, for my battery I chose to get a simple case to hold 4 AA batteries in series. When considering my options, AA batteries don't require specialized charging and are quick, easy, and cheap on a small scale.
I'm still on my first pair of AA's for my project presentation so I think it was a good choice.
