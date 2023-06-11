# DCPU-16
0x10C was a game that was going to feature a user programmable fantasy CPU, the DCPU-16. ASMotor supports assembling code for this CPU.

DCPU-16 specifications can be [found here](https://gist.github.com/metaphox/3888117).

## Word size
This CPU is a little different in that its native word size is 16 bit and it's unable to address individual bytes.

## Section alignment
Sections are aligned to native 16 bit words.

## Command line
### -mo\<x>
This option controls the level of optimization. 0 means no optimization, while 1 will optimize addressing modes to use shorter forms when possible.

## __DCB, __RS, __DSB
|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | | DW | DL |
| Define space | | DSW | |
| Reserve symbol | | RW | RL |


