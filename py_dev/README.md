# /py_dev

- `hci` main file of receiver called by `/display`. Unpacks the packets and sends data to `angle`/`location`
- `angle` module to detect the angle of mouse.
- `location` module to decode the VLC packet.
- `comm_handler`/`setting`/`utils` helper class/funciton