# CWM
CLUNC-Window-Manager
supports: X11, devices with at least 160KB of free memory

# Current Status
~~mostly fucntional but not recommended, I need to optimise the calls to the x server to prevent overload and crash~~

mostly funnctional and I would reccomend if you want something tiny that can run in very little ram, fixed the issue with crashing upon applications automatically killing windows

# Guide
## Window manager designed on the cluncpad for the cluncpad and other systems
## Keybinds:
### Modkey+T = open terminal window
### Modkey+B = open chosen browser
### Modkey+L = switch mode (floating or tiling)
### Modkey+F = make the focussed window the dom window in tiling mode
### Modkey+Tab = Cycle focus right (direction is relative)
### Modkey+Shift+Tab = Cycle focus left (direction is relative)
### Modkey+numbers = change to numbered desktop
### Modkey+leftclick = focus/drag window
### Modkey+rightclick = focus/resize window
### Modkey+(q, a, z, e, c) = quick move window to corner/side of screen 
### Modkey+Shift+C = close focused window
### Modkey+Shift+I = reinitialise config values

config file will be auto generated for you if you lose it, but you will have to set which values you want to be existant. format is pretty straightforward.

# Testimp
testimp.cpp includes  features I am yet to perfect and may be unstable, hence use at your own risk. 
It will be receiveing most of the updates before the main cluncwm.cpp




