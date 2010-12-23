Kaptivate
================================

A library to capture raw mouse and keyboard events under Windows. Based on prior work by [Petr Medek] [1]. Provides both unmanaged C++ and C# bindings. Distributed under the BSD license.

Acknowledgements
-------------------------

Many thanks to [Securics, Inc] [2] and Petr Medek (creator of [HID macros] [1]), withouth whom none of this would have been possible.

Goals
-------------------------

Kaptivate is a library designed to capture keyboard and mouse traffic. Although there are many libraries which are capable of doing this already, this is the first which can do it for all system input events. In other words, the application doesn't have to be in the foreground to capture the events. Furthermore, this library can actually act as a filter of sorts. It is possible to capture (i.e. consume) keystrokes or mouse events. It's also possible to do this on a <em>per device</em> basis.

This library was designed to be a more programmer-friendly version of [HID macros] [1]. Ultimately the goal is to provide a simple API for capturing, consuming, and filtering keystrokes and mouse events. Due to limitations of the underlying API, the library must be written in C / C++. A C# wrapper will be provided for ease of use.

Requirements
-------------------------

*  Visual Studio 2008 or higher


[1]: http://http://www.hidmacros.eu/   "HID macros"
[2]: http://www.securics.com/          "Securics, Inc."
