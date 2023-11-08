#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "link_layer.h"  // Replace with the actual header file you need


#define MAX_PAYLOAD_SIZE 40



unsigned char *MakeCPacket(unsigned char control, unsigned char *filename, long int length, unsigned int *size);
unsigned char *MakeDPacket(unsigned char control, unsigned char *data, int length, unsigned int *size);

#endif