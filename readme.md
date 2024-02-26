# EMI Script

## Build instructions
1. Clone the repository
1. Run Cmake in the root directory and select parameters
1. Build with c++20+ compatible compiler 
1. Run EMICLI project

#### Cmake options

| Option | Description |
| - | - |
| EMI_PARSE_GRAMMAR | Rebuilds the grammar on startup from the .grammar file |
| BUILD_SHARED_LIBS | Builds shared libraries |

## CLI Usage
Command line tool has two stages, compile and scripting.
Scripting stage is used to call functions defined in the script files using syntax  
> function_name arg1 arg2 ... 

**Compile stage commands:**
| Command | Arguments | Info |
| -- |-- | -- | 
| compile | Filepaths separated by spaces | Compiles the given files and enters |scripting stage |
| emi | | Enters the scripting stage |
| reinit | | Reinitializes the grammar if EMI_PARSE_GRAMMAR is defined |
| exit | | Exits the program |