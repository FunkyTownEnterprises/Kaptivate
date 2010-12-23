Kaptivate
================================

A library to capture raw mouse and keyboard events under Windows. Based on prior work by [Petr Medek] [1]. Provides both unmanaged C++ and C# bindings. Distributed under the BSD license.

Acknowledgements
-------------------------

Many thanks to [Securics, Inc] [2] and Petr Medek (creator of [HID macros] [1]), withouth whom none of this would have been possible.

Goals
-------------------------

Kaptivate is a library designed to capture keyboard and mouse traffic. Although there are many libraries which are capable of doing this already, this is the first (as far as we know) which can do it on a <em>per device</em> basis. Keystrokes and mouse events can be captured, processed, and either returned or consumed. This kind of filtering (or monitoring) is possible even when your application is in the background, which means you can actually intercept keystrokes or mouse events before they ever reach another application.

This library was designed to be a more programmer-friendly version of [HID macros] [1]. Ultimately the goal is to provide a simple API for capturing, consuming, and filtering keystrokes and mouse events. Due to limitations of the underlying API, the library must be written in C / C++. A C# wrapper will be provided for ease of use.

Caveats
-------------------------

Due to the Win32 API calls, there is no guarantee that this library will receive an event <strong>first</strong>. If no other applications register for keyboard / mouse hooks (most won't, unless this library becomes popular) things should function as desired.

This library deals in <em>raw key events</em>, NOT digested character messages. At best you will need to translate a virtual keycode to a character. If more complicated modifiers are involved (i.e. shift, ctrl, etc) more advanced trickery will be required. At some point this library may add this functionality, but for now it isn't planned.

References
-------------------------

*  [Hooks Overview](http://msdn.microsoft.com/en-us/library/ms644959\(v=VS.85\).aspx#wh_keyboardhook)
*  [About Raw Input](http://msdn.microsoft.com/en-us/library/ms645543\(v=VS.85\).aspx)
*  [Virtual-Key Codes](http://msdn.microsoft.com/en-us/library/dd375731\(v=VS.85\).aspx)

Requirements
-------------------------

*  Visual Studio 2008 or higher


[1]: http://http://www.hidmacros.eu/   "HID macros"
[2]: http://www.securics.com/          "Securics, Inc."
