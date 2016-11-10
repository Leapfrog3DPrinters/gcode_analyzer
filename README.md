# GCODE Analyzer
GCODE analyzer for 3D printers that determines properties like volume, needed filament, and estimated print time.

## How to build
After cloning this repository into a local directory do:  
``libtoolize && aclocal && autoheader && autoconf && automake --add-missing``  
Then go into the ``src/jansson`` subdirectory and do:  
``libtoolize && aclocal && autoheader && autoconf && automake --add-missing``  
Go back to the root of the local directory and do:  
``./configure``  
``make``  
Optionally you can install the program by doing:  
``make install``  

## How to use
After building the program, type  
``src/gcode_analyze --help``  
Which will then show a list of options. Check out the ``examples`` directory for example config and ignore files.
