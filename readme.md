# EMI Script
Game script in Scripts-folder contains most available features currently implemented.

## Build instructions
1. Clone the repository
1. Run Cmake in the root directory and select parameters
1. Build with c++2x compatible compiler 
1. Run EMICLI or EMIGame binary

#### Cmake options

| Option | Description |
| - | - |
| EMI_PARSE_GRAMMAR | Rebuilds the grammar on startup from the .grammar file |
| BUILD_SHARED_LIBS | Builds shared libraries |

## Command line Usage
Command line tool has two stages, compile and scripting.
Scripting stage is used to call functions defined in the script files using the syntax: 
> function_name arg1 arg2 ... 

**Compile stage commands:**
| Command | Arguments | Info |
| -- |-- | -- | 
| compile | Filepaths separated by spaces | Compiles the given files and enters scripting stage on success |
| emi | | Enters the scripting stage |
|loglevel| integer(0-4) | Sets the current log level (0=Debug, 3=Error, 4=None) |
| reinit | | Reinitializes the grammar if EMI_PARSE_GRAMMAR is defined |
| exit | | Exits the program |

## Game
The demo requires Windows 10 or 11. Windows 11 OS uses different terminal by default and needs the Win11Startup binary.  
  
The game demo starts immediately and loads the game.ril script file automatically. The red dot uses A-star pathfinding to move around the boxed area. Player can draw walls using left mouse button and erase them with right mouse button.  
  
![image](https://github.com/IlkkaTakala/EMI-Script/assets/94061138/ff7d5cd8-ee4e-4dfc-b25a-2562a7b240ab)
