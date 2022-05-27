# Y-maze for animal behavior tests

# Disription

This Arduino project automates rat behavioral tests in Y-maze. It lets experimenter enter different programs 
with 4x4 keypad and LCD 128x64 display. Programs are saved in memory and can be selected and used later.  
Arduino plate controls over LEDs lighting and door opening, it also sends signals to data acquisition interface 
event channel, which lets matching events in the maze with neural activity. Maze has middle LEDs, which draw animal's 
attention, and two LEDs on each side, which clue animal on which way to run.  
Middle LED lights on and off, then side LED lights on and off, then door opens (or stay closed, if thus 
programmed). The times of lighting and delays between signals are determined by current program.

# Hardware

Arduino Mega, LCD 128x64, Keypad 4x4, LED (x5), servo motor, perforated circuit board, 
12V adapter, 12V to 3.3V/5V buck step down power supply module, maze box, (data acquisition interface)

A total of 5 LEDs are used: 3 for middle signal and one for each side. It is important that LEDs are not
too bright, as it can repel the rat. In our experiments we observed steadily wrong choices, as rats, 
even trained to food being on the side of light, avoided it and chose to go to the darker branch. 
It is a good idea to cover the LEDs with some semitransparent material, like paper or matte plastic, 
or at least turn them upward, so that animal looks on it from the side. This way it sees signal, 
but not bright light.

It is also important that the maze box itself is painted matte black from inside. Darker environment 
not only calms a rat and make signals more significant, but also makes sure that there are no reflected 
light that might confuse animal. 

The plate is powered from 220v network through 12V adapter and 3.3V/5V power supply module, which also 
lets turning it on and off and reset. 

## Display connections

Specific for the code. They can be changed in the script and in circuit.

 LCD 128x64  -  pins of Arduino Mega  
 GND - GND  
 VCC - 5v  
 RS - 41  
 R/W - 43  
 E   - 45  
 PSB - GND  
 BLA - 3.3v  
 BLK - GND
 
 ## Keypad connections

There are eight pins on the keypad, which correspond to the following pins on the plate (left to right):  
22 23 26 27 30 31 34 35

# Interface

## Programs' format

**Example 1:**

```
1M3m2S3s5D
```

1 - 1 second delay before middle light  
M - Middle LED on  
3 - Middle LED is on for 3 seconds  
m - Middle LED off  
2 - 2 seconds delay after middle light  
S - Side LED on  
3 - Side LED is on for 3 seconds  
s - Side LED off  
5 - 5 seconds delay after side light  
D - Door opens  

**Example 2:**

```
1M0m0S3s0*
```

In this example time of middle LED is 0, which means it won't light on.  
__*__ instead of **D** means that door won't open.

Side parameter is chosen before each round of experiment:  
__L / R / *__  
**L** stands for left side **for animal**. It is important to remember, as the experimenter is behind
the maze wall and should place food accordingly.  
__*__ is random. Script remembers previous sides and will never give one side **3** times in a row.  
It's made in order to not cause habit to one side in animal.   
This threshold is determined by the following code and can be changed according to the experimental needs:

```
int pickSide(){
  <...>
    if (sameSide >= 3){     // no more than THREE times on the same side in a raw
      <...>
}
```

## Display screens

There are five screens for the whole interface:

1. *First screen*: select program from memory or enter a new one

```
Select program                     A
New program                        D
```

2. *Enter new program*

On this screen user types consecutive numbers of delay times in seconds for each step of program. 
They are spaced by pressing letters on keypad, hinted below on screen.   

Example:

```
1M1m2S_

Enter time of side
C - finish entering                B
```

**B** back to *First screen*  
Entering ends with pressing **#**, which saves new program in memory and sends to *Preparation screen*
with new settings.

3. *Select old program*

```
2. 3M0m0S5s3D                      8
3. 1M1m2S3s0*
4. 1M1m1S1s1D
#  select                       up A
*  delete     	B - back      down D
```

Three programs from the whole list are seen on one screen. Selected is the middle one (framed).  
**8** in the upper right corner shows the number of programs in memory. Max number is 20  
**#** sends to _Preparation screen_ with selected program  
__*__ delets program from memory  
**B** back to *First screen* 

There are two ways to navigate through the list:
- **A/D** buttons shift list up or down one program
- typing program's number on the keypad. If entered number exceeds the number of programs, the last 
one will be selected.

4. *Preparation screen*

```
2M2m2S2s2D


Set rat and door
# - ready                   B - back
```

On the first step rat should be placed behind the door in the starting chamber.  
When **#** is pressed, servo motor closes the door.  
**B** returns to *Program selection*

```
2M2m2S2s2D

A - left
C - right
* - random                  B - back
```

On the second step side should be chosen. Experimenter can either pick side themself or let program
set it randomly. Chosen side will be shown in the upper right corner to let experimenter place food. 
**B** returns to the first step and opens the door.

5. *Experiment screen*
                      
```
2M2m2S2s2D                         L


# - start                   B - back
```

**B** returns to the second screen of preparation and lets change the side.  
**L** shows that left side was chosen. It's the time to place food.   
**#** starts program execution. After that it won't be possible to interrupt it.  
During experiment executed parts of program will be underlined.

# Signal to event channel

When middle and side LEDs light, and when door opens, a 5 ms signal is sent to event channel of data acquisition interface, 
which allows to match events in the maze with animal's neural activity. 

# Bugs

There is one known bug I wasn't yet able to fix. If user goes to *Select old program* screen, then 
returns to the *First screen* and goes to *New program*, pressing any digit will make one of the old
programs appear. It's usually not critical, but if it happens user should reset the maze and go straight
to *New program*.
