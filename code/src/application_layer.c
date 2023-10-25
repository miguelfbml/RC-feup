// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    enum Status device = TRANSMITTER;

    if (strcmp(role, "tx") == 0)
    {
        printf("\n\nMAIN STRCMP RECEIVER\n\n");
        device = RECEIVER;
    }
    else if (strcmp(status_, "rx") == 0)
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
        sleep(3);

        if (llwrite(fd, controlPacketStart, Psize) == -1)
        {
            printf("Exit: error in start packet\n");
            exit(-1);
        }
        sleep(3);
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
        sleep(5);
        if (llwrite(fd, controlPacketEnd, Psize2) == -1)
        {
            printf("Exit: error in exit packet\n");
            exit(-1);
        }
        sleep(5);
        llclose(fd);

        break;

    case RECEIVER:
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
        break;

    default:
        exit(-1);
        break;
    }
    printf("\nCHECKPOINT##\n");
    return 0;

}
