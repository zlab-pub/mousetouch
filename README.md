# MouseTouch

## Main Dirs

- `/display` reads the data from MouseTokens and controls the display content according to signals from `/py_dev`.
- `/py_dev` handles the data and calculates the position/angle of mouse, then send the according signal to `/display`.
- Not only the mouse data but also the control signal is shared by named pipe.

