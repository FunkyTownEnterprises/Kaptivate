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

Example Uses
-------------------------
*  Super easy key logger (let's get that one out of the way)
*  Easily read from card scanners which masquerade as keyboard devices
*  Filter input (prevent other apps from seeing certain keyboard and mouse events)
*  Filter devices (prevent other apps from seeing keyboard and mouse events from certain devices)
*  Take over a specific keyboard or mouse device (by preventing other apps from seeing the input)
*  Mess with people (filter every third 'e' key event)

Caveats
-------------------------

Due to the Win32 API calls, there is no guarantee that this library will receive an event <strong>first</strong>. If no other applications register for keyboard / mouse hooks (most won't, unless this library becomes popular) things should function as desired.

This library deals in <em>raw key events</em>, NOT digested character messages. At best you will need to translate a virtual keycode to a character. If more complicated modifiers are involved (i.e. shift, ctrl, etc) more advanced trickery will be required. At some point this library may add this functionality, but for now it isn't planned.

Finally, and most importantly, this library runs its own thread(s) for processing events. Which means that yes, my dear user, you get to be responsible for thread safety. Luckily there is only one additional thread to worry about (which calls your callback functions), but your code MUST be thread safe. Any handlers you register will execute within Kaptivate's event thread. Please note: if your handlers are slow, the input will feel slow. Things will hold up until you've processed everything.

See the [wiki][3] for a simple example.

References
-------------------------

*  [Hooks Overview](http://msdn.microsoft.com/en-us/library/ms644959\(v=VS.85\).aspx#wh_keyboardhook)
*  [About Raw Input](http://msdn.microsoft.com/en-us/library/ms645543\(v=VS.85\).aspx)
*  [Virtual-Key Codes](http://msdn.microsoft.com/en-us/library/dd375731\(v=VS.85\).aspx)

Requirements
-------------------------

*  Visual Studio 2010 or higher
*  Tested on Windows XP Professional and Windows 7 Professional


[1]: http://www.hidmacros.eu/   "HID macros"
[2]: http://www.securics.com/   "Securics, Inc."
[3]: https://github.com/FunkyTownEnterprises/Kaptivate/wiki "Kaptivate Wiki"
