// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


#define FLAG 0x7E
#define DISC 0x0B
#define ESC 0x7D
#define IN0 0x00
#define IN1 0x40
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
#define TRUE 1
#define FALSE 0

int alarmCount = 0;
int alarmEnabled = FALSE;
int STOP = FALSE;
int flagFrame = 0;
int timeout=0;
int nRetransmissions=0;
int fd=-1;
struct termios oldtio;
int ttt=0;
LinkLayerRole _role;


void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    
    printf("Alarm #%d\n", alarmCount);
}





////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = connectionParameters.timeout*10; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }



    _role = connectionParameters.role;

    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    int cont=0;

    unsigned char buf_w[5] = {FLAG, 0x03, 0x03, 0x03^0x03, FLAG};
    unsigned char buf_r[5] = {FLAG, 0x03, 0x07, 0x03^0x07, FLAG};
    unsigned char c;


    if (_role == LlTx) 
    {
        (void)signal(SIGALRM, alarmHandler);

        while (cont != connectionParameters.nRetransmissions)
        {
            enum State state = START_STATE;

            if (alarmEnabled == FALSE) 
            {
                int bytes = write(fd, buf_w, 5);
                alarm(timeout);
                alarmEnabled = TRUE;
            }
            while (read(fd, &c, 1)>0){
                switch (state) {
                    case START_STATE:
                        if (c==FLAG) state= FLAG_STATE;
                        break;
                    case FLAG_STATE:
                        if (c==0x03) state=A_STATE;
                        else if (c==FLAG) state= FLAG_STATE;
                        else state= START_STATE;
                        break;
                    case A_STATE:
                        if (c==0x07) state = C_STATE;
                        else if (c==FLAG) state= FLAG_STATE;
                        else state= START_STATE;
                        break;
                    case C_STATE:
                        if (c==(0x03^0x07)) state=BCC_STATE;
                        else if (c==FLAG) state= FLAG_STATE;
                        else state= START_STATE;
                        break;
                    case BCC_STATE:
                        if (c==FLAG) return 1;
                        else state= START_STATE;
                        break;


                }
            }
            alarmEnabled=FALSE;
            cont++;
        }
        if (connectionParameters.nRetransmissions==cont){
            printf("UNABLE TO ESTABLISH CONNECTION\n");
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
            {
                perror("tcsetattr");
                exit(-1);
            }
            close(fd);
            return -1;
        }


        alarmEnabled=FALSE;
        alarmCount=0;
        return 1;
    }
    else if (_role == LlRx)
    {

        unsigned char c;
        enum State state=START_STATE;
        while (state != END_STATE){
            read(fd, &c, 1);
            switch (state) {
                case START_STATE:
                    if (c==FLAG) state= FLAG_STATE;
                    break;
                case FLAG_STATE:
                    if (c==0x03) state=A_STATE;
                    else if (c==FLAG) state= FLAG_STATE;
                    else state= START_STATE;
                    break;
                case A_STATE:
                    if (c==0x03) state = C_STATE;
                    else if (c==FLAG) state= FLAG_STATE;
                    else state= START_STATE;
                    break;
                case C_STATE:
                    if (c==(0x03^0x03)) state=BCC_STATE;
                    else if (c==FLAG) state= FLAG_STATE;
                    else state= START_STATE;
                    break;
                case BCC_STATE:
                    if (c==FLAG) {
                        state=END_STATE;
                        write(fd, buf_r, 5);
                    }
                    else state= START_STATE;
                    break;
                case END_STATE:
                    return 1;

            }
        }



        return 1;
    }

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }


    printf("error"); 
    return -1;

}



unsigned char state_write(){
    enum State state = START_STATE;
    unsigned char read2, save;

    while (state!=END_STATE){
        if(read(fd, &read2, 1)>0){
            switch (state) {
                case START_STATE:
                    if (read2==FLAG)state=FLAG_STATE;
                    break;
                case FLAG_STATE:
                    if (read2==0x03) state=A_STATE;
                    else if (read2==FLAG) state=FLAG_STATE;
                    else state=START_STATE;
                    break;
                case A_STATE:
                    if (read2==RR0){
                        flagFrame=0;
                        save=read2;
                        state=C_STATE;
                    }
                    else if(read2==RR1){
                        flagFrame=1;
                        save=read2;
                        state=C_STATE;
                    }
                    else if(read2==REJ0){
                        flagFrame=0;
                        save=read2;
                        state=C_STATE;
                    }
                    else if (read2==REJ1){
                        flagFrame=1;
                        save=read2;
                        state=C_STATE;
                    }
                    else if (read2==DISC) save=read2;
                    else if (read2==FLAG) state=FLAG_STATE;
                    else state=START_STATE;
                    break;
                case C_STATE:
                    if (read2==(0x03^save))state= BCC_STATE;
                    else if (read2==FLAG) state=FLAG_STATE;
                    else state=START_STATE;
                    break;
                case BCC_STATE:
                    if (read2==FLAG)state = END_STATE;
                    else state=START_STATE;
                    break;
                default:
                    break;
            }

        }
        else break;
    }

    return save;

}





////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{

    (void)signal(SIGALRM, alarmHandler);

    // TODO

    if (fd < 0) {
        perror("Invalid file descriptor");
        return -1;
    }

    unsigned char* stuffed = (unsigned char*)malloc(2 * bufSize * sizeof(unsigned char));
    stuffed[0]=FLAG;
    stuffed[1]=0x03;
    if (flagFrame==0){
        stuffed[2]=IN0;
    }
    else if (flagFrame==1){
        stuffed[2]=IN1;
    }
    stuffed[3]=stuffed[1]^stuffed[2];
    int cont=4;
    unsigned char xor=0;
    for(int i=0; i<bufSize; i++){
        xor^=buf[i];
        if (buf[i]==0x7e){
            stuffed[cont]=ESC;
            stuffed[cont+1]=0x5e;
            cont+=2;
        }
        else if (buf[i]==0x7d){
            stuffed[cont]=ESC;
            stuffed[cont+1]=0x5d;
            cont+=2;
        }
        else{
            stuffed[cont]=buf[i];
            cont++;
        }
    }


    stuffed[cont]=xor;
    stuffed[cont+1]=FLAG;

    cont+=2;


    int trans = nRetransmissions;


    int bytes=0;
    alarmEnabled=FALSE;
    alarmCount=0;


    while (trans!=0){
        if (alarmEnabled == FALSE)
        {
            bytes = write(fd, stuffed, cont);
            alarm(timeout);
            alarmEnabled = TRUE;
        }
        if (bytes != cont) {
            fd=-1;
            perror("Error writing to serial port");
            return -1;
        }
        unsigned char read = state_write();

        if (read==RR1 || read==RR0){
            alarm(0);
            break;
        }
        else if (read==REJ0 || read==REJ1){
            alarm(0);
        }
        alarmEnabled=FALSE;
        trans--;

    }


    if (trans==0){
        return -1;
    }

   return bytes;


}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char* packet)
{

    unsigned char bytes;
    enum State state = START_STATE;
    unsigned char c_bye;
    int i=0;


    while (state!=END_STATE){
        if (read(fd, &bytes, 1)<0){
            return 1;
        }
        switch (state) {
            case START_STATE:
                if (bytes == FLAG) state=FLAG_STATE;
                break;
            case FLAG_STATE:
                if (bytes==0x03) state=A_STATE;
                else if (bytes==FLAG) state=FLAG_STATE;
                else state=START_STATE;
                break;
            case A_STATE:
                if (bytes==IN0 || bytes==IN1){
                    state = C_STATE;
                    c_bye = bytes;
                    if (bytes==IN0) flagFrame=0;
                    else flagFrame=1;
                }
                else if (bytes==FLAG) state=FLAG_STATE;
                else state=START_STATE;
                break;
            case C_STATE:
                if (bytes== (0x03^c_bye))state=DATA_STATE;
                else if (bytes==FLAG) state=FLAG_STATE;
                else state=START_STATE;
                break;
            case DATA_STATE:
                if (bytes==FLAG){
                    unsigned char BCC2 = packet[i-1];
                    unsigned char xor=0;
                    for (int j=0; j<i-1; j++){
                        xor^=packet[j];
                    }

                    if (xor==BCC2) {
                        state = END_STATE;
                        if (flagFrame == 0) {
                            unsigned char send_RR1[5] = {FLAG, 0x03, RR1, 0x03 ^ RR1, FLAG};
                            write(fd, send_RR1, 5);
                            return i-1;
                        }
                        else if (flagFrame==1){
                            unsigned char send_RR0[5] = {FLAG, 0x03, RR0, 0x03 ^ RR0, FLAG};
                            write(fd, send_RR0, 5);
                            return i-1;
                        }
                    }
                    else{
                        printf("REJECTED\n");
                        if (flagFrame == 0) {
                            unsigned char send_REJ0[5] = {FLAG, 0x03, REJ0, 0x03 ^ REJ0, FLAG};
                            write(fd, send_REJ0, 5);
                            return -1;
                        } else if (flagFrame == 1) {
                            unsigned char send_REJ1[5] = {FLAG, 0x03, REJ1, 0x03 ^ REJ1, FLAG};
                            write(fd, send_REJ1, 5);
                            return -1;
                        }
                    }
                }
                else if (bytes==ESC) state = ESC_STATE;
                else packet[i++]=bytes;
                break;
            case ESC_STATE:
                state=DATA_STATE;
                if (bytes==0x5e) packet[i++]=0x7e;
                else if (bytes==0x5d) packet[i++]=0x7d;
                else{
                    packet[i++]=ESC;
                    packet[i++]=bytes;
                }
                break;

        }

    }

    return -1;

}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    (void)signal(SIGALRM, alarmHandler);

    alarmEnabled=FALSE;
    alarmCount=0;
    unsigned char buf_disc[5] = {FLAG, 0x03, 0x0B, 0x03^0x0B, FLAG};
    unsigned char buf_ua[5] = {FLAG, 0x03, 0x07, 0x03^0x07, FLAG};
    unsigned char rec;
    int cont=0;
    unsigned char c;


    if (_role==LlTx) {
        (void) signal(SIGALRM, alarmHandler);
        enum State state = START_STATE;


        while (cont != nRetransmissions && state != END_STATE) {

            if (alarmEnabled == FALSE) {
                int bytes = write(fd, buf_disc, 5);
                alarm(timeout);
                alarmEnabled = TRUE;
            }
            while (state != END_STATE) {
                read(fd, &c, 1);
                switch (state) {
                    case START_STATE:
                        if (c == FLAG) state = FLAG_STATE;
                        break;
                    case FLAG_STATE:
                        if (c == 0x01) state = A_STATE;
                        else if (c == FLAG) state = FLAG_STATE;
                        else state = START_STATE;
                        break;
                    case A_STATE:
                        if (c == DISC) state = C_STATE;
                        else if (c == FLAG) state = FLAG_STATE;
                        else state = START_STATE;
                        break;
                    case C_STATE:
                        if (c == (0x01 ^ 0x0B)) state = BCC_STATE;
                        else if (c == FLAG) state = FLAG_STATE;
                        else state = START_STATE;
                        break;
                    case BCC_STATE:
                        if (c == FLAG) {
                            state = END_STATE;
                            alarm(0);
                        } else state = START_STATE;
                        break;


                }
            }
            alarmEnabled = FALSE;
            cont++;
        }

        write(fd, buf_ua, 5);
    }

    else if (_role==LlRx) {
        unsigned char bytes;
        enum State state_r = START_STATE;
        unsigned char c_bye;
        int i = 0;

        unsigned char sv;

        while (state_r != END_STATE) {
            
            read(fd, &bytes, 1);

            switch (state_r) {
                case START_STATE:
                    if (bytes == FLAG) state_r = FLAG_STATE;
                    break;
                case FLAG_STATE:
                    if (bytes == 0x03) state_r = A_STATE;
                    else if (bytes == FLAG) state_r = FLAG_STATE;
                    else state_r = START_STATE;
                    break;
                case A_STATE:
                    if (bytes == DISC) {
                        sv=bytes;
                        state_r=C_STATE;
                    } else if (bytes == FLAG) state_r = FLAG_STATE;
                    else if (bytes==0x07){
                        state_r=C_STATE;
                        sv=bytes;
                    }
                    else state_r = START_STATE;
                    break;
                case C_STATE:
                    if (bytes== (0x03^sv))state_r=BCC_STATE;
                    else state_r=START_STATE;
                    break;
                case BCC_STATE:
                    if (bytes==FLAG){
                        if (sv==DISC){
                            unsigned char send_disc[5] = {FLAG, 0x01, DISC, 0x01 ^ DISC, FLAG};
                            write(fd, send_disc, 5);
                            state_r = START_STATE;
                        }
                        else if (sv==0x07){
                            state_r=END_STATE;
                        }
                    }
                    break;

            }
        }
        
    }

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    return close(fd);

}
