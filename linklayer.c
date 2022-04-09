#include "linklayer.h"

struct applicationLayer
{
  int fileDescriptor; /*Descritor correspondente à porta série*/
  int status;         /*TRANSMITTER | RECEIVER*/
};

//Define Flags, Header and Trailer
#define FLAG 0x07
#define A_T 0x03
#define A_R 0x01
#define SET 0x03
#define DISC 0x0b
#define UA 0x07
#define BBC1_T A_T^SET  //Será usado o mesmo Address no llopen, tanto para Transmitter como Receiver
#define BCC1_R A_T^UA

/***********************
 * Struct Linklayer received values:
 * ll.serialPort="/dev/ttySx"
 * ll.role=0 se tx, 1 se rx
 * ll.baudRate=9600
 * ll.numTries=3
 * ll.timeOut=3
 * ****************************/

//needs to be global variable, but honestly tenho de ver outra maneira que a stora disse que dava
int fd;
int state=0;


void tictoc(){
  printf("Timedout bro\n");
	state=0;
}



// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters){

    struct termios oldtio, newtio;

    //Open connection
    if (((strcmp("/dev/ttyS10", connectionParameters.serialPort)!=0) && 
  	      (strcmp("/dev/ttyS11", connectionParameters.serialPort)!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(connectionParameters.serialPort); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    //Comunicaçao aberta



    
    int k=0, res=0;
    char input[5], output[5];
    
    //activates timeOut interrupt
    (void) signal(SIGALRM, tictoc);
    
    
    switch(connectionParameters.role){

        case TRANSMITTER:

        input[0]=FLAG;
        input[1]=A_T;
        input[2]=SET;
        input[3]=BBC1_T;
        input[4]=FLAG;

  while(k<=connectionParameters.numTries && state!=6){
  switch(state){

    //escrever informaçao e verificar
    case 0:  
        alarm(0);       //reinicia alarm
        printf("Estamos em state=0 pela %d vez\n",k);
        
        if(k>connectionParameters.numTries){
        printf("Nao recebeste dados nenhuns amigo, problems de envio\n");break;
        }

        k++;
        res=write(fd,input,5);
        printf("%d Bytes written\n", res);
        state=1;
        alarm(connectionParameters.timeOut);    
    break;

    //receber informaçao
    case 1:  
		printf("Entramos na 1º funçao read\n");
		while((!read(fd,&output[0],1)) && state){}
		printf("saimos do primeiro read\n");
		if(output[0]==FLAG){
			printf("Lemos a prImeira flag :)\n");
			state=2;
            alarm(0);       //paramos o timer, porque de facto temos uma receçao de dados
		}else state=0;
    
    break;
  
  case 2:
  while(!read(fd,&output[1],1)){}
  if(output[1]==A){
	 printf("Address very well received!\n");
	 state=3;
	  }else state=0;
	  
break;	  
	  
	  case 3:
while(!read(fd,&output[2],1)){}
	if(output[2]==UA){
    printf("UA IS HERE WITH US TOOOOO!\n");
    state=4;
}else state=0;
    
    
break;    
    case 4: 
    while(!read(fd,&output[3],1)){}
      if(output[3]==BCC_R){
    printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
    state=5;
  }else state=0;printf("Erros de transmissao, repeat please bro!\n");
    
break;    
    case 5:
    while(!read(fd,&output[4],1)){}
    if(output[4]==Flag){
    printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");state=6;
  }else {state=0;printf("Erros de transmissao, repeat please bro!\n");}
    
    break;
    
  

  }
  }
  printf("WE DID IT MOTHERFUCKERS\n");



        break;




        case RECEIVER:

        input[0]=FLAG;
        input[1]=A_T;
        input[2]=UA;
        input[3]=BCC1_R;
        input[4]=FLAG;





        break;
    }




}
