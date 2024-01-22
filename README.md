# RCOM


Project done for the [FEUP Computer Networking course](https://sigarra.up.pt/feup/pt/ucurr_geral.ficha_uc_view?pv_ocorrencia_id=520330)

The goal of the project was to create the code to transfer a file from one computer to another using Series Port RS-232. 

In order to run the code, you have to open a terminal to start virtual series port, then open two terminals, one as a sender and the other as the receiver. You can try to run the code with our example, using the pinguin image.  

First terminal window
> Command: $sudo socat -d  -d  PTY,link=/dev/ttyS10,mode=777   PTY,link=/dev/ttyS11,mode=777

Second terminal window in the same directory as the source code:
> Command #1: $gcc *.c -o exe

> Command #2: $./exe /dev/ttyS11 rx penguin-received.gif

Third terminal window in the same directory as the source code:
> Command: $./exe /dev/ttyS10 tx penguin.gif