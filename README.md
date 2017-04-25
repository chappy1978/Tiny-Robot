# Tiny-Robot
Tiny line follower robot designed to use the Attiny family of ICs

This version deals with some of the short comings of earlier versions. First, thing was the readablity of the different
functions, switches replaced many of the if-else statements and clear #DEFINEs used to identify what actions should occur.

The second, was a tendency for the Robot to run to fast at first. When the battery is first topped off and then stop 
working to early. This is due to the fact the Robot does not use a regulator in the design. The set duty cycle off the PWM
to the motors does not supply enough current to keep it moving. So code was added to up the duty cycle to keep it going till 
the battery is exhausted. Hope you enjoy.

