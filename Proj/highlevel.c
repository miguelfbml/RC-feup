#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <signal.h>
#include <stdio.h>

#include <stdbool.h>


#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5 //256


#define FLAG 0x7E 
#define A_RECEIVER 0x01
#define A_TRANSMITER 0x03
#define SET 0x03
#define UA 0x07
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
#define DISC 0x0B
#define INFORMATION_FRAME0 0x00
#define INFORMATION_FRAME1 0x0B


#define C_I(Ns) (Ns << 6);

#define ESC 0x7d
#define FLAG_R 0x5e
#define ESC_R 0x5d

enum rec_status {Start, flag_rcv, a_rcv, c_rcv, bcc_ok, a_tx, c_tx, STOP};

int fd;

enum Status {RECEIVER, TRANSMITER}; 


int alarmEnabled = FALSE;
int alarmCount = 0;


int Ns = 0;
int Nr = 1;



struct termios oldtio;
struct termios newtio;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}

int sendcontrol(unsigned char frame2, unsigned char frame3){
    unsigned char frame[5] = {0}; 
    frame[0] = FLAG;
    frame[1] = frame2;
    frame[2] = frame3;
    frame[3] = frame2 ^ frame3;
    frame[4] = FLAG;
    write(fd, frame, 5);
    return 0;
}



int setup(const char* serialPortName){
    fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }


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
    newtio.c_cc[VTIME] = 5; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(sCLOC)

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
    return 0;
}


int llopen(const char* porta, enum Status status)  {

    printf("\n\n LLOPEN status %i \n\n", status);


    if (setup(porta) < 0) return -1;
    


    enum rec_status state_mach_rec = Start; 

    unsigned char byte = 0;
    bool pr = (status == RECEIVER);
    printf("\n bool:%i \n",pr);
    
    switch (status)
    {
    case RECEIVER:
        printf("\n\nentrou recetor LLOPEN\n\n");

        while(state_mach_rec != STOP){
            if (read(fd, &byte, 1) > 0){
                switch (state_mach_rec)
                {
                case Start:
                    if (byte == FLAG){ state_mach_rec = flag_rcv; }
                    //else {state_mach_rec = Start;}
                    break;
                case flag_rcv:
                    if(byte == A_TRANSMITER){ state_mach_rec = a_rcv; }
                    else if (byte == FLAG) { state_mach_rec = flag_rcv; }
                    else {state_mach_rec = Start;}
                    break;
                case a_rcv:
                    if(byte == SET){ state_mach_rec = c_rcv; }
                    else if (byte == FLAG){ state_mach_rec = flag_rcv; }
                    else {state_mach_rec = Start;}
                    break;
                case c_rcv:
                    if(byte == (A_TRANSMITER ^ SET)){ state_mach_rec = bcc_ok; }
                    else if (byte == FLAG){ state_mach_rec = flag_rcv; }
                    else {state_mach_rec = Start;}
                    break;
                case bcc_ok:
                    printf("\nSTOP NEXT\n");
                    if(byte == FLAG){ state_mach_rec = STOP; }
                    else {state_mach_rec = Start;}
                    break;
                default:
                    break;
                }
                    
            }
            
        }

    sendcontrol(A_RECEIVER, UA);
    break;

    case TRANSMITER:
        printf("\n\nentrou trasmiter LLOPEN\n\n");
        /* code */
        (void)signal(SIGALRM, alarmHandler);

        enum rec_status state_mach_tx = Start; 

        while (alarmCount < 3 && state_mach_tx != STOP)
        {
            
            sendcontrol(A_TRANSMITER, SET);
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;

            //so volta aqui passado 3 segundos

            while(alarmEnabled == TRUE && state_mach_tx != STOP){
                if (read(fd,&byte,1) > 0){
                    
                    switch (state_mach_tx)
                {
                case Start:
                    if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    //else {state_mach_tx = Start;}
                    break;
                case flag_rcv:
                    if(byte == A_RECEIVER){ state_mach_tx = a_tx; }
                    else if (byte == FLAG) { state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case a_tx:
                    if(byte == UA){ state_mach_tx = c_tx; }
                    else if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case c_tx:
                    if(byte == (A_RECEIVER ^ UA)){ state_mach_tx = bcc_ok; }
                    else if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case bcc_ok:
                    printf("\nSTOP NEXT\n");
                    if(byte == FLAG){ state_mach_tx = STOP; }
                    else {state_mach_tx = Start;}
                    break;
                default:
                    break;
                }
                }
            }
            alarm(0);
        }
        alarmCount = 0;
        
    break;
    default:
        break;
    }


    return fd;
}


//return –number of writer characters –negative value in case of error
int llwrite(int fd, char * buffer, int length) {
    int frame_len = length + 6;

    unsigned char *frame = (unsigned char *) malloc(frame_len * sizeof(char));

    frame[0] = FLAG;
    frame[1] = A_TRANSMITER;
    frame[2] = C_I(Ns);
    frame[3] = frame[1] ^ frame[2];

    unsigned char bcc2 = buffer[0];
    for (int i = 1; i < length; i++){
        bcc2 ^= buffer[i];
    }

    //memccpy(frame+4,buffer,length);

    int pointer_f = 4;

    for (int i = 0; i < length; i++){

        if (buffer[i] == FLAG){
            frame_len++;
            frame = realloc(frame, frame_len);
            frame[pointer_f++] = ESC;
            frame[pointer_f++] = FLAG_R;
            continue;
        }

        else if (buffer[i] == ESC){
            frame_len++;
            frame = realloc(frame, frame_len);
            frame[pointer_f++] = ESC;
            frame[pointer_f++] = FLAG_R;
            continue;
        }

        else {

            frame[pointer_f++] = buffer[i];
        }
        

    }

    frame[pointer_f++] = bcc2;
    frame[pointer_f] = FLAG;

    enum rec_status state_mach_tx = Start; 
    bool accepted = 0;
    bool rejected = 0;
    char byte = 0;
    char byte_c = 0;
    
    while (alarmCount < 3 && state_mach_tx != STOP)
        {
            
            write(fd,frame,frame_len);
            //sendcontrol(A_TRANSMITER, SET);
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;

            //so volta aqui passado 3 segundos

            while(alarmEnabled == TRUE && state_mach_tx != STOP){
                if (read(fd,&byte,1) > 0){
                    
                    switch (state_mach_tx)
                {
                case Start:
                    if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    //else {state_mach_tx = Start;}
                    break;
                case flag_rcv:
                    if(byte == A_RECEIVER){ state_mach_tx = a_tx; }
                    else if (byte == FLAG) { state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case a_tx:                   //new
                    byte_c = byte;
                    if(byte == RR0 || byte == RR1){ state_mach_tx = c_tx; 
                        accepted = 1;
                        Ns = (Ns + 1) % 2;
                    }
                    else if (byte == REJ0 || byte == REJ1){ state_mach_tx = c_tx; 
                        rejected = 1;
                    }
                    else if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case c_tx:
                    if(byte == (A_RECEIVER ^ byte_c)){ state_mach_tx = bcc_ok; }
                    else if (byte == FLAG){ state_mach_tx = flag_rcv; }
                    else {state_mach_tx = Start;}
                    break;
                case bcc_ok:
                    if(byte == FLAG){ state_mach_tx = STOP; }
                    else {state_mach_tx = Start;}
                    break;
                default:
                    break;
                }

                }
    
            }

        alarm(0);
        if (rejected) {printf("rejected"); rejected = 0; state_mach_tx = Start; continue;}
        if (accepted) {printf("accepted"); break;}

        }
        
        alarmCount = 0;
        free(frame);

        if (accepted > 0) return frame_len;
        else {
            llclose(fd);   
            return -1;
        }

}



//return –array length (number of characters read) –negative value in case of error
int llread(int fd, char * buffer){}








int main(int argc, char *argv[]){
    if (argc < 3)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }




    const char *serialPortName = argv[1];
    const char *status_ = argv[2];

    enum Status teste = TRANSMITER;

    if (strcmp(status_, "recetor") == 0) {
        printf("\n\nMAIN STRCMP RECEIVER\n\n");
        teste = RECEIVER;
    }
    else if (strcmp(status_,"emissor") == 0){
        printf("\n\nMAIN STRCMP TRANSMITTER\n\n");
        teste = TRANSMITER;
   }

    //setup(serialPortName);



    printf("%i",llopen(serialPortName, teste));





    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);


    return 0;
}



