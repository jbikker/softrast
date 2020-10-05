BRIEF INFO ON THE 2018-02 TEMPLATE

Purpose:
The template has been designed to make it easy to start coding C++
using games and graphics. It intends to offer the programmer a
simple library with the main purpose of providing a 32-bit graphical
window with a linear frame buffer. Some basic additional functionality
is available, such as sprites, bitmap fonts, basic multi-threading,
and vector math support.

How to use:
1. Copy the template folder (or extract the zip) to a fresh folder for
   your project. 
2. Open the .sln file with any version of Visual Studio 2015.
3. Replace the example code in game.cpp with your own code.
4. Copy the 64-bit dll's from dlls_x64 to the project folder if you
   want to run a 64-bit build.
You can go further by:
- Expanding the game class in game.h;
- Implementing some of the empty functions for mouse and keyboard
  handling;
- Exploring the code of the template in surface.cpp and template.cpp.

When handing in assignments based on this template, please run
clean.bat prior to zipping the folder. This deletes any intermediate
files created during compilation.

The Template is a 'quickstart' template, and not meant to be elaborate
performant or complete. 
At some point, and depending on your requirements, you may want to
advance to a more full-fledged library, or you can expand the template
with OpenGL or SDL2 code.

Credits
Although the template is small and bare bones, it still uses a lot of
code gathered over the years:
- EasyCE's 5x5 bitmap font (primarily used for debugging);
- EasyCE's surface class (with lots of modifications);
- Nils Desle's JobManager for efficient multi-threading;
- This version of the template uses SDL2 for low-level window handling,
  hopefully improving on the compatibility of earlier versions.

Copyright
This code is completely free to use and distribute in any form.

Remplate Naming
Starting January 2018, the name of the template represents the version.
This version also appears in the title of the window. Make sure you
are using the most recent version.

Utrecht, 2015-2018, Utrecht University - Breda, 2014, NHTV/IGAD
Jacco Bikker
Report problems and suggestions to bikker.j@gmail.com .

Changelog

v2018-01: 
initial DGDARC release.

v2018-02:
added a changelog.
debug mode now also emits .exe to project folder.
removed rogue SDL2 folder.
added assert.h to precomp.h.