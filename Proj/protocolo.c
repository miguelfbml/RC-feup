
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#include <signal.h>
#include <stdio.h>

#include <stdbool.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5 // 256

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

// ruben

// ruben 2

#define C_I(Ns) (Ns << 6)

#define C_RR(Nr) ((Nr << 7) | 0x05)
#define C_REJ(Nr) ((Nr << 7) | 0x01)

#define ESC 0x7d
#define FLAG_R 0x5e
#define ESC_R 0x5d

#define C_DISC 0x0B

int delay = 3;
int numretransmitions = 3;

enum rec_status
{
    Start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
    a_tx,
    c_tx,
    STOP,
    read_data,
    found_esc,
    next_esc
};

int fd;

enum Status
{
    RECEIVER,
    TRANSMITER
};

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

int sendcontrol(unsigned char frame2, unsigned char frame3)
{
    unsigned char frame[5] = {0};
    frame[0] = FLAG;
    frame[1] = frame2;
    frame[2] = frame3;
    frame[3] = frame2 ^ frame3;
    frame[4] = FLAG;
    for (int i = 0; i < 5; i++)
        printf("-%x-", frame[i]);
    write(fd, frame, 5);
    return 0;
}

int setup(const char *serialPortName)
{
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

int llopen(const char *porta, enum Status status)
{

    printf("\n\n LLOPEN status %i \n\n", status);

    if (setup(porta) < 0)
        return -1;

    enum rec_status state_mach_rec = Start;

    unsigned char byte = 0;
    bool pr = (status == RECEIVER);
    printf("\n bool:%i \n", pr);

    switch (status)
    {
    case RECEIVER:
        printf("\n\nentrou recetor LLOPEN\n\n");

        while (state_mach_rec != STOP)
        {
            if (read(fd, &byte, 1) > 0)
            {
                switch (state_mach_rec)
                {
                case Start:
                    if (byte == FLAG)
                    {
                        state_mach_rec = flag_rcv;
                    }
                    // else {state_mach_rec = Start;}
                    break;
                case flag_rcv:
                    if (byte == A_TRANSMITER)
                    {
                        state_mach_rec = a_rcv;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_rec = flag_rcv;
                    }
                    else
                    {
                        state_mach_rec = Start;
                    }
                    break;
                case a_rcv:
                    if (byte == SET)
                    {
                        state_mach_rec = c_rcv;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_rec = flag_rcv;
                    }
                    else
                    {
                        state_mach_rec = Start;
                    }
                    break;
                case c_rcv:
                    if (byte == (A_TRANSMITER ^ SET))
                    {
                        state_mach_rec = bcc_ok;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_rec = flag_rcv;
                    }
                    else
                    {
                        state_mach_rec = Start;
                    }
                    break;
                case bcc_ok:
                    if (byte == FLAG)
                    {
                        state_mach_rec = STOP;
                    }
                    else
                    {
                        state_mach_rec = Start;
                    }
                    printf("\nbyte: %i\n", state_mach_rec);
                    break;
                default:
                    break;
                }
            }
            printf("loop");
        }
        printf("\nmandou receiver\n");
        sendcontrol(A_RECEIVER, UA);
        break;

    case TRANSMITER:
        printf("\n\nentrou trasmiter LLOPEN\n\n");
        /* code */
        (void)signal(SIGALRM, alarmHandler);

        enum rec_status state_mach_tx = Start;

        alarmCount = 0;

        while (alarmCount < numretransmitions && state_mach_tx != STOP)
        {

            sendcontrol(A_TRANSMITER, SET);
            alarm(delay); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;

            // so volta aqui passado 3 segundos

            while (alarmEnabled == TRUE && state_mach_tx != STOP)
            {
                if (read(fd, &byte, 1) > 0)
                {

                    switch (state_mach_tx)
                    {
                    case Start:
                        if (byte == FLAG)
                        {
                            state_mach_tx = flag_rcv;
                        }
                        // else {state_mach_tx = Start;}
                        break;
                    case flag_rcv:
                        if (byte == A_RECEIVER)
                        {
                            state_mach_tx = a_tx;
                        }
                        else if (byte == FLAG)
                        {
                            state_mach_tx = flag_rcv;
                        }
                        else
                        {
                            state_mach_tx = Start;
                        }
                        break;
                    case a_tx:
                        if (byte == UA)
                        {
                            state_mach_tx = c_tx;
                        }
                        else if (byte == FLAG)
                        {
                            state_mach_tx = flag_rcv;
                        }
                        else
                        {
                            state_mach_tx = Start;
                        }
                        break;
                    case c_tx:
                        if (byte == (A_RECEIVER ^ UA))
                        {
                            state_mach_tx = bcc_ok;
                        }
                        else if (byte == FLAG)
                        {
                            state_mach_tx = flag_rcv;
                        }
                        else
                        {
                            state_mach_tx = Start;
                        }
                        break;
                    case bcc_ok:
                        printf("\nSTOP NEXT\n");
                        if (byte == FLAG)
                        {
                            state_mach_tx = STOP;
                        }
                        else
                        {
                            state_mach_tx = Start;
                        }
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

// argumentos –fd: identificador da ligação de dados retorno –valor positivo em caso de sucesso –valor negativo em caso de erro
int llclose(int fd)
{

    unsigned char byte;
    enum rec_status state_mach_tx = Start;

    alarmCount = 0;

    sleep(1);
    while (alarmCount < numretransmitions && state_mach_tx != STOP)
    {

        sendcontrol(A_TRANSMITER, DISC);
        alarm(delay); // Set alarm to be triggered in 3s
        alarmEnabled = TRUE;

        // so volta aqui passado 3 segundos

        printf("\n llclose recebe:\n");
        while (alarmEnabled == TRUE && state_mach_tx != STOP)
        {
            if (read(fd, &byte, 1) > 0)
            {

                printf("-%x-", byte);

                switch (state_mach_tx)
                {
                case Start:
                    if (byte == FLAG)
                    {
                        state_mach_tx = flag_rcv;
                    }
                    // else {state_mach_tx = Start;}
                    break;
                case flag_rcv:
                    if (byte == A_RECEIVER)
                    {
                        state_mach_tx = a_rcv;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_tx = flag_rcv;
                    }
                    else
                    {
                        state_mach_tx = Start;
                    }
                    break;
                case a_rcv:
                    if (byte == DISC)
                    {
                        state_mach_tx = c_rcv;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_tx = flag_rcv;
                    }
                    else
                    {
                        state_mach_tx = Start;
                    }
                    break;
                case c_rcv:
                    if (byte == (A_RECEIVER ^ DISC))
                    {
                        state_mach_tx = bcc_ok;
                    }
                    else if (byte == FLAG)
                    {
                        state_mach_tx = flag_rcv;
                    }
                    else
                    {
                        state_mach_tx = Start;
                    }
                    break;
                case bcc_ok:
                    if (byte == FLAG)
                    {
                        state_mach_tx = STOP;
                    }
                    else
                    {
                        state_mach_tx = Start;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        alarm(0);
    }

    alarmCount = 0;

    if (state_mach_tx != STOP)
        return -1;
    printf("\n SENDING UA\n");
    sleep(1);
    sendcontrol(A_TRANSMITER, UA);

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return 0;
}

// return –number of writer characters –negative value in case of error
int llwrite(int fd, char *buffer, int length)
{
    int frame_len = length + 6;

    unsigned char *frame = (unsigned char *)malloc(frame_len * sizeof(char));

    frame[0] = FLAG;
    frame[1] = A_TRANSMITER;
    frame[2] = C_I(Ns);
    frame[3] = frame[1] ^ frame[2];

    unsigned char bcc2 = buffer[0];
    for (int i = 1; i < length; i++)
    {
        bcc2 ^= buffer[i];
    }

    // memccpy(frame+4,buffer,length);

    int pointer_f = 4;

    for (int i = 0; i < length; i++)
    {

        if (buffer[i] == FLAG)
        {
            frame_len++;
            frame = realloc(frame, frame_len);
            frame[pointer_f++] = ESC;
            frame[pointer_f++] = FLAG_R;
            continue;
        }

        else if (buffer[i] == ESC)
        {
            frame_len++;
            frame = realloc(frame, frame_len);
            frame[pointer_f++] = ESC;
            frame[pointer_f++] = ESC_R;
            continue;
        }

        else
        {

            frame[pointer_f++] = buffer[i];
        }
    }

    //frame[pointer_f++] = bcc2;

    //
    if (bcc2 == FLAG)
    {
        frame_len++;
        frame = realloc(frame, frame_len);
        frame[pointer_f++] = ESC;
        frame[pointer_f++] = FLAG_R;
    }

    else if (bcc2 == ESC)
    {
        frame_len++;
        frame = realloc(frame, frame_len);
        frame[pointer_f++] = ESC;
        frame[pointer_f++] = ESC_R;
    }

    else {frame[pointer_f++] = bcc2;}
    //


    /*

    if (frame[pointer_f] == FLAG){
        frame_len++;
        frame = realloc(frame, frame_len);
        frame[pointer_f++] = ESC;
        frame[pointer_f++] = FLAG_R;

    }

    */


    frame[pointer_f] = FLAG;

    enum rec_status state_mach_tx = Start;
    bool accepted = 0;
    bool rejected = 0;
    unsigned char byte = 0;
    unsigned char byte_c = 0;

    int curr_retransmition = 0;


    printf("emissor envia: \n");
    for (int i = 0; i < frame_len; i++){
        printf("-%x-",frame[i]);
    }

    printf("\n\n\n");
    printf("emissor recebe: \n");
    while (curr_retransmition < numretransmitions && state_mach_tx != STOP)
        {

            write(fd,frame,frame_len);
            //sendcontrol(A_TRANSMITER, SET);
            alarm(delay); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;

            //so volta aqui passado 3 segundos

            while(alarmEnabled == TRUE && state_mach_tx != STOP){
                if (read(fd,&byte,1) > 0){
                    printf("-%x-",byte);

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
        curr_retransmition++;
        if (rejected && state_mach_tx == STOP) {printf("rejected"); rejected = 0; state_mach_tx = Start; continue;}
        if (accepted && state_mach_tx == STOP) {printf("accepted"); accepted = 0; break;}
        rejected = 0;
        accepted = 0;

        }

        alarmCount = 0;
        free(frame);

        if (accepted > 0) return frame_len;
        else {
            printf("\n not accepted \n");
            //llclose(fd);
            return -1;
        }

}










//–fd:       identificador da ligação de dados –buffer: array de caracteres recebidos
//return –array length (number of characters read) –negative value in case of error
int llread(int fd, char * buffer){



    enum rec_status state_mach_rec = Start;
    unsigned char byte;
    unsigned char c_byte;

    int i = 0;

    unsigned char b_byte;

    printf("\n\n STARING LLREAD: \n\n");
    while(state_mach_rec != STOP){
            if (read(fd, &byte, 1) > 0){
                printf("-%x-",byte);
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
                    if(byte == C_I(0) || byte == C_I(1)){ state_mach_rec = c_rcv; c_byte = byte;}
                    else if (byte == C_DISC) { ////////////// aqui
                        c_byte = C_DISC;
                        state_mach_rec = c_rcv;
                        printf(" \n FOI DETETADO NO READ ANTES \n");
                    }
                    else if (byte == UA){
                        printf("\n\n /////UA RECEIVED /////\n\n");
                        c_byte = UA;
                        state_mach_rec = c_rcv;
                    }
                    else if (byte == FLAG){ state_mach_rec = flag_rcv; }
                    else {state_mach_rec = Start;}
                    break;
                case c_rcv:
                    if ( byte == (A_TRANSMITER ^ c_byte) && c_byte == DISC){ state_mach_rec = bcc_ok; printf("\n está certo !!!!! \n");}
                    else if ( byte == (A_TRANSMITER ^ c_byte) && c_byte == UA) {state_mach_rec = bcc_ok; }

                    else if( byte == (A_TRANSMITER ^ c_byte)){ state_mach_rec = read_data; } //pode ler a informacao

                    else if ( byte == FLAG){ state_mach_rec = flag_rcv; }
                    else {state_mach_rec = Start;}
                    break;


                case read_data:
                    if (byte == ESC) state_mach_rec = next_esc;
                    else if (byte == FLAG){
                        unsigned char bcc2 = buffer[i-1];

                        buffer[--i] = '\0';

                        printf("\n buffer está: \n");
                        for (int z = 0; z < i; z++){
                            printf("-%x-",buffer[z]);
                        }

                        unsigned char bcc_try = buffer[0];

                        for (int j = 1; j < i; j++ ){
                            bcc_try ^= buffer[j];
                        }

                        printf("recetor envia: \n");
                        if (bcc_try == bcc2){
                            state_mach_rec = STOP;
                            sendcontrol(A_RECEIVER,C_RR(Nr));
                            Nr = (Nr + 1) % 2;
                            return i;
                        }

                        else {
                            sendcontrol(A_RECEIVER, C_REJ(Nr));
                            return -1;
                        }

                    }
                    else {buffer[i++] = byte;}

                    break;

                case next_esc:
                    printf("esc exception: %x\n",byte);
                    state_mach_rec = read_data;
                    if (byte == FLAG_R)
                        buffer[i++] = FLAG;
                    else if (byte == ESC_R)
                        buffer[i++] = ESC;
                    break;

                case bcc_ok:
                    printf("\nno bcc ok está: %x \n ",byte);
                    if (byte == FLAG) {
                        state_mach_rec = STOP;
                    }

                    break;


                default:
                    break;
                }

            }

        }



        if (c_byte == C_DISC){
            printf("\n READ SENT DISC TO LLCLOSE \n");
            sendcontrol(A_RECEIVER, C_DISC);
        }



        if (c_byte == UA){

        sleep(1);

        if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
        {
            perror("tcsetattr");
            exit(-1);
        }

        printf("\n\n////////////CLOSING/////////////// \n\n");
        close(fd);

        }





    return -1;
}



#define MAX_PAYLOAD_SIZE 40


unsigned char * MakeCPacket(unsigned char control, char *filename, long int length, unsigned int *size){


    int L1 = (int) ceil(log2f((float)length)/8.0);    //L1 numero de bytes necessário para representar o numero length em hexadecimal 
    int L2 = strlen(filename);
    int sizP = 1 + 2 + L1 + 2 + L2;
    *size = sizP;
    unsigned char * packet = (unsigned char *) malloc(sizP);
    
    int pos = 0;
    packet[pos++] = 2;
    packet[pos++] = 0;
    packet[pos++] = L1;
    

    for (int i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = length & 0xFF;
        length >>= 8;
    }

    pos+=L1;

    packet[pos++]= 1;

    packet[pos++]=L2;

    memcpy(packet+pos, filename, L2);
    return packet;
}



unsigned char * MakeDPacket(unsigned char control, unsigned char *data, int length, unsigned int *size){
    *size = 1 + 2 + length;
    unsigned char* packet = (unsigned char*)malloc(*size);


    packet[0] = 1;   
    packet[1] = (length >> 8) & 0xFF;
    packet[2] = length & 0xFF;
    memcpy(packet+3, data, length);

    return packet;
}





int main(int argc, char *argv[])
{




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
    char *filename = "pinguim.gif";


    enum Status teste = TRANSMITER;

    if (strcmp(status_, "recetor") == 0) {
        printf("\n\nMAIN STRCMP RECEIVER\n\n");
        teste = RECEIVER;
    }
    else if (strcmp(status_,"emissor") == 0){
        printf("\n\nMAIN STRCMP TRANSMITTER\n\n");
        teste = TRANSMITER;
   }



    int fd = llopen(serialPortName, teste);
        if (fd < 0) {
        perror("Connection error\n");
        exit(-1);
    }


    switch (teste)
    {
    case TRANSMITER:
        
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
            perror("File not found\n");
            exit(-1);
        }


        long int fileSize0 = ftell(file);
        fseek(file, 0L, SEEK_END);
        long int fileSize = ftell(file) - fileSize0;
        fseek(file, 0L, SEEK_SET);

        unsigned int Psize;
        unsigned char *controlPacketStart = MakeCPacket(2, filename, fileSize, &Psize);


        if(llwrite(fd, controlPacketStart, Psize) == -1){ 
                printf("Exit: error in start packet\n");
                exit(-1);
        }

        free(controlPacketStart);

        unsigned char sequence = 0;
        unsigned char* file_content = (unsigned char*) malloc(sizeof(unsigned char) * fileSize);
        fread(file_content, sizeof(unsigned char), fileSize, file);
        long int bytesLeft = fileSize;

        //need to free file_content



        while (bytesLeft >= 0) { 

                int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
                unsigned char* data = (unsigned char*) malloc(dataSize);
                //need to free data
                memcpy(data, file_content, dataSize);



                int packetSize;
                unsigned char* packet = MakeDPacket(sequence, data, dataSize, &packetSize);
                //need to free packet;
                
                if(llwrite(fd, packet, packetSize) == -1) {
                    printf("Exit: error in data packets\n");
                    exit(-1);
                }
                
                bytesLeft -= (long int) MAX_PAYLOAD_SIZE; 
                file_content += dataSize; 
                //sequence = (sequence + 1) % 255;   
            }


            unsigned int Psize2;
            unsigned char *controlPacketEnd = MakeCPacket(2, filename, fileSize, &Psize2);


            if(llwrite(fd, controlPacketEnd, Psize2) == -1){ 
                    printf("Exit: error in exit packet\n");
                    exit(-1);
            }

            llclose(fd);


        break;


    case RECEIVER:

        unsigned char *packet_r = (unsigned char *) malloc(MAX_PAYLOAD_SIZE);
        int packet_rSize = 0;
        while ((packet_rSize = llread(fd, packet_r)) < 0);

        unsigned long int newFileSize = 0;

        //processar o packet recebido

        unsigned char FileSize_NBytes = packet_r[2];
        
        //unsigned char aux[FileSize_NBytes];
        unsigned char *aux = (unsigned char *) malloc(FileSize_NBytes);


        memcpy(aux, packet_r + 3, FileSize_NBytes);


        for(unsigned int i = 0; i < FileSize_NBytes; i++)
        newFileSize |= (aux[FileSize_NBytes-i-1] << (8*i));
        //tamanho feito


        unsigned char Name_NBytes = packet_r[3+FileSize_NBytes+1];
        unsigned char* nome;
        memcpy(nome, packet_r + 3 + FileSize_NBytes + 2, Name_NBytes);
        //nome feito


        memcpy(nome + Name_NBytes, "_Novo", 5);

        FILE* newFile = fopen((char *) nome, "wb+");

        //continuar
        //a partir daqui
        while (1) {    
                while ((packet_rSize = llread(fd, packet_r)) < 0);
                if(packet_rSize == 0) break;
                else if(packet_r[0] != 3){
                    unsigned char *buffer = (unsigned char*)malloc(packet_rSize);

                    memcpy(buffer, packet_r + 4, packet_rSize - 4);
                    buffer += packet_rSize + 4;

                    fwrite(buffer, sizeof(unsigned char), packet_rSize-4, newFile);
                    free(buffer);
                }
                else continue;
        }


        fclose(newFile);
        break;




    
    default:
        exit(-1);
        break;

    }





    return 0;



}














