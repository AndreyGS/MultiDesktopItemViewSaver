# MultiDesktopItemViewSaver

Current project is evolution (and if saying more accurate - remake) of respective app from the book of J.Richter and C.Nasarre "Windows VIA C/C++". That program was a pretty simple and had a few bugs and simplifications. My program is not so complex too, but I fixed that just mentioned bugs and added some more functionality. In particular, now it can save and restore configuration (saved in registry) for each unique combination of desktop displays.

For make this working I impliment two auxillary classes - first for retriving general information about displays and second for connecting previous one with registry settings.

The usage of program is simple enough.
---------------------------------------------------

Almost forgoten to tell what it actually does... This program saves and restores desktop items position. If you save your current layout, you might change somehow display configuration and when you switch back, you'll be able to restore your previous configuration saved with help of this program. As I already mentioned multiple display configurations supported. Configuration distinguishes by resolution, mutual dispostion or taskbar properties (height/weight and position) of every plugged display. If one or more of these settings are different in regard to saved one, items arrangement is saved in separate profile.

The program was tested on Windows 7x64 and 10x64

## License

Copyright (c) 2020 Andrey Grabov-Smetankin

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.