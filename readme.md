# Willis
![Dr. Willis - Ghost in the Shell gif](https://user-images.githubusercontent.com/5473047/80808503-fbef4d80-8bbf-11ea-98d0-0b74498a3afe.gif)

Willis is a lightweight input library for keyboard and mouse events. It was
designed to communicate directly with display systems whithout relying on a
particular windowing library or widget toolkit. This flexibility makes it
perfectly suited for use with custom windowing or low-abstraction libraries like
[Globox](https://github.com/nullgemm/globox).

## Design
The main design goal was to provide the same API on all supported platforms,
exposing only the common input features of X11, Wayland, Win32 and AppKit.

Under Wayland, several technical limits make it mandatory to call the getters
and utilitary functions from the event callback only.

For the same reasons, additional Willis contexts may only be initialized
sequentially under Wayland. This means a new Willis callback may only be
registered and the corresponding context initialized after the last
initialization function has returned.

### Events
The mouse events supported by Willis are the following:
 - Mouse buttons presses (left, right and middle)
 - Mouse wheel steps (up and down)
 - Mouse movements

As for the keyboard all PC-104 keys are handled except "ScrollLock" and "Pause"
which were disabled because they are not available on macOS.

On this same amazingly compatible platform, pressing "NumLock" will impact the
keycodes returned by using the numeric keypad in a way we can't work around
(the keypad is assigned duplicate codes, from other already existing keys).
Because of that, using "NumLock" is discouraged as well (but possible).

### States
Willis sends events with an associated state, describing wether a key or mouse
button was pressed or released. Mouse movements and wheel steps come with a
third, neutral state.

### Data
When a movement event is received, it is possible to get `mouse_x` and `mouse_y`
using the appropriate functions to retrieve the current mouse position.
When a keypress is received and if the utf-8 string feature was enabled when
initializing Willis, it is possible to get `utf8_string` and `utf8_size`.
Character composition is fully supported by Willis, so dead-keys can succesfully
produce diacritics on all target platforms without the need for another library.

## Usage
### Integration
#### X11
Enable the required events with `xcb_change_window_attributes_checked`:
 - XCB_EVENT_MASK_KEY_PRESS
 - XCB_EVENT_MASK_KEY_RELEASE
 - XCB_EVENT_MASK_BUTTON_PRESS
 - XCB_EVENT_MASK_BUTTON_RELEASE
 - XCB_EVENT_MASK_POINTER_MOTION;

Forward XCB events to `willis_handle_events`, which will summon your Willis callback
(the function you passed to `willis_init`, with its associated data pointer).
More details in XCB'x [documentation](https://xcb.freedesktop.org/)
and in Globox' [XCB](https://github.com/nullgemm/globox/blob/willis/src/globox_x11.c)
backend.

#### Wayland
Handle `wl_seat_interface` in your global registry callback:
 - Bind the seat with `wl_registry_bind`
 - Pass the returned seat to `willis_handle_events`, which will perform some
   extra black magic initialization in order to make your Willis callback the
   client's default input handler

More details in the Wayland online [book](https://wayland-book.com)
and in Globox' [Wayland](https://github.com/nullgemm/globox/blob/willis/src/globox_wayland.c)
backend.

#### Windows and MacOS
Simply pass input events to `willis_handle_events`, which runs your callback.
More details in Globox' [Win32](https://github.com/nullgemm/globox/blob/willis/src/globox_win.c)
and [AppKit](https://github.com/nullgemm/globox/blob/willis/src/globox_quartz.c)
backends.

### Callback
The Willis event callback is given the following as arguments:
 - A pointer to the Willis callback to allow you to get mouse positions
   and composed utf-8 text input
 - A Willis event code
 - A Willis event state
 - The custom data pointer you passed in `willis_init`

```
void callback(
	struct willis* willis,
	enum willis_event_code event_code,
	enum willis_event_state event_state,
	void* data)
{
	// handle willis events
}
```

All event code and state values can be found in `willis_events.h`.
You can also use `debug.c` to get their name as regular strings:
this is how they are logged in the Globox [example](https://github.com/nullgemm/globox/blob/willis/src/main_willis.c).

### Initialization
After integrating Willis and declaring your callback it is time to call
`willis_init`, with the following arguments:
 - A pointer to the Willis context
 - A pointer to XCB's connection descriptor (or NULL if running another backend)
 - A boolean variable describing wether composed text input should be enabled
 - Your callback's function pointer
 - The custom data pointer that will be passed to your callback

```
bool willis_init(
	struct willis* willis,
	void* backend_link,
	bool utf8,
	void (*callback)(
		struct willis* willis,
		enum willis_event_code event_code,
		enum willis_event_state event_state,
		void* data),
	void* data);
```

## Greetings
Development fueled by Deltron3030 and a.u.t.o.m.a.t.o.r.
