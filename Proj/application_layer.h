#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "link_layer.h"  // Replace with the actual header file you need


#define MAX_PAYLOAD_SIZE 40


//cria um pacote de controlo
unsigned char *MakeCPacket(unsigned char control, unsigned char *filename, long int length, unsigned int *size);
//cria um pacote de dados
unsigned char *MakeDPacket(unsigned char control, unsigned char *data, int length, unsigned int *size);

#endif