# RC

## Proj1 - FileTransfer

### Running

Go to the project folder

run on the terminal to simulate two computers with serial port rs232 connection.
* sudo socat -d  -d  PTY,link=/dev/ttyS10,mode=777   PTY,link=/dev/ttyS11,mode=777

run **make** on the terminal.
* make

run to initiate the tranference.
* ./program /dev/ttyS11 recetor
* ./program /dev/ttyS10 emissor


You should see a new file called pinguin_novo.gif.