// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>


unsigned char* saved = NULL;
int globalSize=0;

double totalTime=0;




unsigned char* getControlPacket(int c, int oc, unsigned char* buffer){
    unsigned char* packet_ret = (unsigned char*) malloc (oc+3);

    packet_ret[0]=c;
    packet_ret[1]=0x00;
    packet_ret[2]=oc;

    for (int i=0; i<oc; i++){
        packet_ret[i+3]=buffer[i];
    }

    return packet_ret;


}


unsigned char* getDataPacket(int bytesRead, unsigned char* read){

    unsigned char* buffer = (unsigned char*) malloc (MAX_PAYLOAD_SIZE);


    buffer[0] = 0x01;
    buffer[1] = (int)bytesRead / 256;
    buffer[2] = (int)bytesRead%256;
    for (int i = 0; i < bytesRead; i++) {
        buffer[i+3] = read[i];
    }
    int total = (int)bytesRead;
    total+=3;


    return buffer;
}




int divide(const char* filename) {


    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }


    unsigned char *buffer = (unsigned char *) malloc(MAX_PAYLOAD_SIZE-3 * sizeof(unsigned char));
    unsigned char *buffer2 = (unsigned char *) malloc((MAX_PAYLOAD_SIZE) * sizeof(unsigned char));

    size_t bytesRead;

    fseek(file, 0L, SEEK_END);
    int sz = ftell(file);
    fseek(file, 0L, SEEK_SET);
    
    
    unsigned char octets[2];

    octets[0]=sz/256;
    octets[1]=sz%256;


    unsigned char *buffer3 = getControlPacket(2, 2, octets);


    struct timespec start_time, end_time;
    double elapsed_time;

    clock_gettime(CLOCK_REALTIME, &start_time);  // Record the start time


    if (llwrite(buffer3, 5) < 0) {
        return -1;
    }

    clock_gettime(CLOCK_REALTIME, &end_time);    // Record the end time

    elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    totalTime+=elapsed_time;
    printf("Time spent in llWrite: %.2f seconds\n", elapsed_time);
    double debito = (2+9)/elapsed_time;
    printf("Debito recebido: %f\n", debito);

    free(buffer3);


    while ((bytesRead = fread(buffer, 1, MAX_PAYLOAD_SIZE-3, file))>0){

        buffer2 = getDataPacket((int)bytesRead, buffer);

        int total = (int)bytesRead +3 ;


        clock_gettime(CLOCK_REALTIME, &start_time);  // Record the start time

        if ((llwrite(buffer2, total))<0){
            printf("Error sending the message");
            free(buffer);
            free(buffer2);
            fclose(file);
            return -1;
        }
        clock_gettime(CLOCK_REALTIME, &end_time);    // Record the end time

        elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        totalTime+=elapsed_time;
        printf("Time spent in llWrite: %.2f seconds\n", elapsed_time);
        double debito = (((int)bytesRead) + 6)/elapsed_time;
        printf("Debito recebido: %f\n", debito);

    }

    unsigned char *buffer4 = getControlPacket(3, 2, octets);

    clock_gettime(CLOCK_REALTIME, &end_time);    // Record the end time
    if (llwrite(buffer4, 2+3) < 0) {
        return -1;
    }
    clock_gettime(CLOCK_REALTIME, &end_time);    // Record the end time

    elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    totalTime+=elapsed_time;
    printf("Time spent in llWrite: %.2f seconds\n", elapsed_time);
    debito = (((int)bytesRead) + 6)/elapsed_time;
    printf("Debito recebido: %f\n", debito);

    free(buffer2);
    free(buffer);
    fclose(file);
    return 1;

}




unsigned char* packet_rewrite(unsigned char* packet, int size){

    unsigned char* ret = (unsigned char*) malloc((size-3));

    for (int i=3;i<size; i++){
        ret[i-3]=packet[i];
    }

    return ret;

}




void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{


    LinkLayer connectionParameters;


    if (strcmp(role, "tx") == 0){
        connectionParameters.role = LlTx;
    }
    else if ((strcmp(role, "rx") == 0)){
        connectionParameters.role = LlRx;
    }
    else{
        printf("Invalid role, please try again.");
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;
    strcpy(connectionParameters.serialPort, serialPort);




    if (llopen(connectionParameters)!=-1) {

        if (strcmp(role, "tx") == 0) {
            divide(filename);
            printf("tempo total = %f\n", totalTime);
            llclose(10);
            printf("\n\nFile successfully sent\n");

        } else if ((strcmp(role, "rx") == 0)) {

            FILE* newFile = fopen((char *) "penguin-received.gif", "wb+");

            unsigned char* packet = (unsigned char*)malloc(MAX_PAYLOAD_SIZE);
            int packetSize=-1;

            while((packetSize= llread(packet))<0);


             while(1){
                while((packetSize= llread(packet))<0);
                if (packet[0]==0x03){
                    break;

                }
                globalSize+=packetSize-3;
                unsigned char* new = packet_rewrite(packet, packetSize);
                fwrite(new, 1, packetSize-3, newFile);
             }

             printf("global size: %d\n", globalSize);
             fclose(newFile);
             llclose(10);
             printf("\n\nFile successfully received\n");

        }
    }

}

