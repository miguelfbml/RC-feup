
#include "application_layer.h"
#include <time.h>

unsigned char *MakeCPacket(unsigned char control, unsigned char *filename, long int length, unsigned int *size)
{
    printf("\nCHECKPOINT#5\n");
    printf("\n LENGTH: %ld\n", length);
    int L1 = (int)ceil(log2f((float)length) / 8.0); // L1 numero de bytes necessário para representar o numero length em hexadecimal
    int L2 = strlen(filename);
    printf("\nSIZE_L1:%d\n", L1);
    printf("\nSIZE_L2:%d\n", L2);
    int sizP = 1 + 2 + L1 + 2 + L2;
    *size = sizP;
    printf("\nSIZE_P:%d\n", sizP);
    unsigned char *packet = (unsigned char *)malloc(sizP);
    int pos = 0;
    packet[pos++] = control;
    packet[pos++] = 0;
    packet[pos++] = L1;

    // 2 length

    for (int i = 0; i < L1; i++)
    {
        packet[2 + L1 - i] = length & 0xFF;
        length >>= 8;
    }

    pos += L1;

    packet[pos++] = 1;

    packet[pos++] = L2;
    printf("\nCHECKPOINT#7\n");
    memcpy(packet + pos, filename, L2);
    return packet;
}

unsigned char *MakeDPacket(unsigned char control, unsigned char *data, int length, unsigned int *size)
{
    *size = 1 + 2 + length;
    unsigned char *packet = (unsigned char *)malloc(*size);

    packet[0] = 1;
    packet[1] = (length >> 8) & 0xFF;
    packet[2] = length & 0xFF;
    memcpy(packet + 3, data, length);

    return packet;
}






int main(int argc, unsigned char *argv[])
{

    if (argc < 3)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort> <device>\n"
               "Example: %s /dev/ttyS1 recetor\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // unsigned char * delt = (unsigned char *) malloc(2);
    // delt[0] = 'a';
    // delt[1] = 'b';

    // printf("\n\n  %s \n\n",delt);

    const unsigned char *serialPortName = argv[1];
    const unsigned char *status_ = argv[2];
    unsigned char *filename = "pinguim.gif";

    enum Status device = TRANSMITTER;

    if (strcmp(status_, "recetor") == 0)
    {
        printf("\n\nMAIN STRCMP RECEIVER\n\n");
        device = RECEIVER;
    }
    else if (strcmp(status_, "emissor") == 0)
    {
        printf("\n\nMAIN STRCMP TRANSMITTER\n\n");
        device = TRANSMITTER;
    }

    int fd = llopen(serialPortName, device);
    printf("\nCHECKPOINT#2\n");
    if (fd < 0)
    {
        perror("Connection error\n");
        exit(-1);
    }
    printf("\nCHECKPOINT#3\n");

    double cpu_time_used;
    clock_t start, end;


    switch (device)
    {
    case TRANSMITTER:

        FILE *file = fopen(filename, "rb");
        if (file == NULL)
        {
            perror("\nFile not found\n");
            exit(-1);
        }

        long int fileSize0 = ftell(file);
        fseek(file, 0L, SEEK_END);
        long int fileSize = ftell(file) - fileSize0;
        fseek(file, 0L, SEEK_SET);
        unsigned int Psize;
        printf("\nFILE_SIZE: %ld \n", fileSize);
        unsigned char *controlPacketStart = MakeCPacket(2, filename, fileSize, &Psize);
        printf("\n//////////\n//////////\n//////////\nPACKET START WILL WRITE\n//////////\n//////////\n//////////");


        if (llwrite(fd, controlPacketStart, Psize) == -1)
        {
            printf("Exit: error in start packet\n");
            exit(-1);
        }

        printf("\n PACKET_CONTROL FREE \n");
        free(controlPacketStart);

        unsigned char sequence = 0;
        unsigned char *file_content = (unsigned char *)malloc(sizeof(unsigned char) * fileSize);
        fread(file_content, sizeof(unsigned char), fileSize, file);
        long int bytesLeft = fileSize;

        // need to free file_content

        while (bytesLeft >= 0)
        {

            int dataSize = bytesLeft > (long int)MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
            unsigned char *data = (unsigned char *)malloc(dataSize);
            // need to free data
            memcpy(data, file_content, dataSize);

            int packetSize;
            unsigned char *packet = MakeDPacket(sequence, data, dataSize, &packetSize);
            // need to free packet;

            if (llwrite(fd, packet, packetSize) == -1)
            {
                printf("Exit: error in data packets\n");
                exit(-1);
            }
            //sleep(1);
            
            bytesLeft -= (long int)MAX_PAYLOAD_SIZE;
            file_content += dataSize;
            // sequence = (sequence + 1) % 255;
        }

        unsigned int Psize2;
        unsigned char *controlPacketEnd = MakeCPacket(3, filename, fileSize, &Psize2);
        printf("\n//////////\n//////////\n//////////\nPACKET END WILL WRITE\n//////////\n//////////\n//////////");

        if (llwrite(fd, controlPacketEnd, Psize2) == -1)
        {
            printf("Exit: error in exit packet\n");
            exit(-1);
        }

        llclose(fd);

        break;

    case RECEIVER:
        start = clock();
        printf("\n\nRECEIVER#1\n\n");
        unsigned char *packet_r = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int packet_rSize = 0;
        while ((packet_rSize = llread(fd, packet_r)) < 0)
            ;
        printf("\nAAAAAAAAAAAAAA\nAAAAAAAAAAA\nAAAAAAAAAAAAAA\n->CONTROL PACKET: %d\n", packet_r[0]);
        unsigned long int newFileSize = 0;

        // processar o packet recebido

        unsigned char FileSize_NBytes = packet_r[2];

        // unsigned char aux[FileSize_NBytes];
        unsigned char *aux = (unsigned char *)malloc(FileSize_NBytes);

        memcpy(aux, packet_r + 3, FileSize_NBytes);
        printf("\n\nRECEIVER#2\n\n");
        for (unsigned int i = 0; i < FileSize_NBytes; i++)
            newFileSize |= (aux[FileSize_NBytes - i - 1] << (8 * i));
        // tamanho feito

        unsigned char Name_NBytes = packet_r[3 + FileSize_NBytes + 1];
        unsigned char *nome = (unsigned char *)malloc(Name_NBytes + 9);
        memcpy(nome, packet_r + 3 + FileSize_NBytes + 2, Name_NBytes);
        memcpy(nome + Name_NBytes - 4, "_novo.gif", 9);
        printf("\nNOME:%s\n", nome);
        // nome feito

        FILE *newFile = fopen((unsigned char *)nome, "wb+");
        printf("\n\nRECEIVER#3\n\n");

        while (1)
        {

            // stuck
            while ((packet_rSize = llread(fd, packet_r)) < 0)
            {
            }
            //printf("\nREAD PACKET\n");
            printf("\n//////////\n//////////\n//////////\nPACKET 0: %d\n//////////\n//////////\n//////////", packet_r[0]);
            if (packet_rSize == 0){
                printf("Packet size is 0\n");
                break;
            }
            else if (packet_r[0] != 3)
            {
                unsigned char *buffer = (unsigned char *)malloc(packet_rSize);

                memcpy(buffer, packet_r + 3, packet_rSize - 3);
                //buffer += packet_rSize + 4;
                
                fwrite(buffer, sizeof(unsigned char), packet_rSize - 3, newFile);
                printf("\n BUFFER FREE \n");
                free(buffer);
            }
            else
            {
                printf("\n//////////\n//////////\n//////////\nBREAK PACKET %d\n//////////\n//////////\n//////////", packet_r[0]);
                printf("\n\nRECEIVER WHILE\n\n");
                continue;
            }
        }

        fclose(newFile);
        end = clock;
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        break;

    default:
        exit(-1);
        break;
    }
    printf("\nCHECKPOINT##\n");
    return 0;
}
