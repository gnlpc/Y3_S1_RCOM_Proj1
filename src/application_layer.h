// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_
#define MAX_PAYLOAD_SIZE 1000



// Function called by divide to build a control packet
// Arguments: 
//          c: argument to put in control field 
//          oc: parameter to put in L1 and L2
//          buffer: L1 and L2
unsigned char* getControlPacket(int c, int oc, unsigned char* buffer);



// function called by the divide to build the data packet to send 
// Arguments:
//           read: pointer with the data to transfer
//           bytesRead: size of bytes read saved in the pointer read
unsigned char* getDataPacket(int bytesRead, unsigned char* read);




// Function called by the Application Layer  to divide and send the packet to the llwrite
// Arguments: filename: name of tthe file to be opened and sent to the receiver
// returns -1 on error and 1 on success
//  
int divide(const char* filename);


// Function called by the Application layer to remove the first three elements of the packet, which are the control field, and two fields that are the number of octets that llread function read
// Arguments: packet: packet read by the llread
//            size: size of the packet
// returns a new unsigned cha* with all the elements except the first three 
unsigned char* packet_rewrite(unsigned char* packet, int size);

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

#endif // _APPLICATION_LAYER_H_
