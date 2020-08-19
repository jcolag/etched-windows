# etched-windows

A Virtual Etch-A-Sketch for Windows '95 systems (because it's that old)

"Welcome."

Well, it's not a big deal, really, except for the fact that most
of it seems to work and there are some ill-advised features.

I don't know if there's even a way to compile this, any more, but if there is and anybody at Ohio Art trying to use this, the name "Etch-A-Sketch" is used here without any permission whatsoever from anybody (at least, not from anyone who's permission would make any difference in court).  I'm not intending to infringe on anything; it's just for fun, and a bit of a tribute to the original digital graphics hardware.

Brand-Spankin'-New features (as of, like, 2004) include:
 * [x] Interface to programming language modules (see Help)
 * [x] Adds (and accepts) files from the Start Menu's list of recent documents
 * [x] Similarly with File menu, plus an associated registry entry to control the length of the list
 * [x] Support for the "Wheel Mouse," to independently control Vertical Movement
 * [x] Very primitive Groupware functionality/Collaborative mode (complete with some...interesting bugs)

Nifty new features include:
 * [x] Cosmetic Touchup
 * [x] A LOGO-like interpreter to draw circles and the like
 * [x] Registry-Configurable Color, Socket Polling Rate, and Dial Speed

Old, nifty new features include:
 * [x] Command-line arguments for filenames and for port configuration, and toggling options for shaking, sticky cursors, and "Unsaved File" popups
 * [x] Port configuration can be done through the command line or \Windows\Services file (but still has default)

Really old, not-quite-as-nifty-anymore, new features include:
 * [x] "Sticky" Cursor and Cursor Blink/Flash
 * [x] Shaking the Etch-A-Sketch is now optional
 * [x] Primitive Sockets interface (using Winsock 2.0)
 * [x] Primitive Drawing Interchange File (*.DXF) export (for CAD programs)
 * [x] Keyboard controls (for the mouse-impaired)
 * [x] A semi-useful help file
 * [x] Win32 Compliance--this is a full 32-bit application!

Future enhancements will include (as time permits, of course):
 * [ ] Image overlays
 * [ ] More export formats
 * [ ] Startup configuration files
 * [ ] Maybe a Multiple Document Interface

You can feel free to send me suggestions and bug reports.  I may or may not do something about them, if I can ever find a compiler that works.

**2020 Note**:  There's a decent chance this wasn't my final version, so I've added the `test` folder with some additional code from a CVS repository that I'm pretty sure were eventually added.  At this writing, I have not sifted through any of it to determine its usefulness.
