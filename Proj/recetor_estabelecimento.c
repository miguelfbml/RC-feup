// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5 //256

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    unsigned char buffer_receive[BUF_SIZE] = {0};
    unsigned char buffer_send[BUF_SIZE] = {0};

    while (STOP == FALSE) {
        int bytes = read(fd, buffer_receive, BUF_SIZE);
        //buffer_receive[bytes] = '\0';

        printf("frame que recebeu\n");
        printf(":%d:\n", buffer_receive[0]);
        printf(":%d:\n", buffer_receive[1]);
        printf(":%d:\n", buffer_receive[2]);
        printf(":%d:\n", buffer_receive[3]);
        printf(":%d:\n", buffer_receive[4]);
        if (buffer_receive[0] == 0x7E &&
            buffer_receive[1] == 0x03 &&
            buffer_receive[2] == 0x03 &&
            buffer_receive[3] == (buffer_send[1] ^ buffer_send[2]) &&
            buffer_receive[4] == 0x7E) 
        {

        buffer_send[0] = 0x7E;
        buffer_send[1] = 0x01;
        buffer_send[2] = 0x07;
        buffer_send[3] = (buffer_send[1] ^ buffer_send[2]);
        buffer_send[4] = 0x7E;

        printf("frame de envio\n");
        printf(":%d:\n", buffer_send[0]);
        printf(":%d:\n", buffer_send[1]);
        printf(":%d:\n", buffer_send[2]);
        printf(":%d:\n", buffer_send[3]);
        printf(":%d:\n", buffer_send[4]);


        //bytes = write(fd, buffer_send, BUF_SIZE);
        STOP = TRUE;
        }
    }




    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
