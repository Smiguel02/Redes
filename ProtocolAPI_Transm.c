#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

//Biblioteca para abrir a API como transmissor

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TIMEOUT 3

#define ESCBYTE 0x7d
#define MAX_SIZE 255
#define TRANSMITTER 1
#define RECEIVER 0

//Supervision/Unnumbered frames
//novos defines para maquina de estados
#define Flag 0x7e
#define A 0x03 //Transmitter
#define SET 0x03
#define UA 0X07
#define BCC_T A^SET
#define BCC_R A^UA
#define C 0x02


  int state=0;

volatile int STOP=FALSE;

struct applicationLayer {
int fileDescriptor; /*Descritor correspondente à porta série*/
int status; /*TRANSMITTER | RECEIVER*/
};

struct linkLayer {
char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
int baudRate; /*Velocidade de transmissão*/
unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1, ...*/
unsigned int timeout;   /*Valor do temporizador: 1 s*/
unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
char frame[MAX_SIZE]; /*Trama*/
};

  struct applicationLayer app;  



//Inicia comunicaçao entre transmitter(util=0) e receiver(util=1)
//returns data connection id, or negative value in case of failure/error

void tictoc(){
  printf("Timedout bro\n");
	state=0;
}


int llopen(int porta){

  int i, res,  fd, k=0;
  char *door=malloc(sizeof(char));
  char input[5],output[5];
  struct linkLayer layer;
  struct termios oldtio,newtio;



  //define port
  *door='0'+porta;
  strcpy(layer.port, "/dev/ttyS");
  if(porta<10){
  strcat(layer.port, door);
  }else if(porta==10){
    strcat(layer.port, "10");
  }else     strcat(layer.port, "11");
  
  layer.baudRate=BAUDRATE;
  layer.timeout=3;
  layer.numTransmissions=3;

  //Criar informação num buffer de 5 elementos
    input[0]=Flag;
    input[1]=A;
    input[2]=SET;
    input[3]=BCC_T;
    input[4]=Flag;

  //abrir porta
  //COnfirma se inserimos bem no kernel
    if ((strcmp("/dev/ttyS0", layer.port)!=0) && 
  	      (strcmp("/dev/ttyS11", layer.port)!=0)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open ser  printf("Port: %s")
ial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/
  
  {
    fd = open(layer.port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(layer.port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */



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


  }
    
  //inicializar alarme
  (void) signal(SIGALRM, tictoc);



  //Maquina de estados para verificação da mensagem
  k=0;
  while(k<=layer.numTransmissions && state!=6){
  switch(state){

    //escrever informaçao e verificar
    case 0:  
		alarm(0);
		printf("Estamos em state=0 pela %d vez\n",k);
    if(k>layer.numTransmissions){
      printf("Nao recebeste dados nenhuns amigo, problems de envio\n");break;
    }
    k++;
    res=write(fd,input,5);
    printf("%d Bytes written\n", res);
    state=1;
    alarm(layer.timeout);    
break;

    //receber informaçao
    case 1:  
		printf("ENtramos na funçao read\n");
		while(!read(fd,&output[0],1)){}
		printf("saimos do primeiro read\n");
		if(output[0]==Flag){
			printf("Lemos a promeira flag\n");
			state=2;
			}else state=0;
    
    
break;
  //verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
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
  app.fileDescriptor=fd;
  return 1;

}

/*

int llwrite(char* buf, int bufSize){
  //iniciar porta para questoes de teste


  int  i=0, aux, k=0, inputSize=bufSize+4;
  int stuffedSize;
  char *input=malloc(sizeof(char)*(inputSize)]);
  char *stuffed=malloc(sizeof(char)*(bufSize*2+8));
  char bcc2=0;
  input[0]=A;
  input[1]=C;
  input[2]=A^C;


//BCC2 creation
//to be honest n sei de onde vem esta ideia do XOR, so we must ask teacher
for(i=0;i<bufSize;i++){
  bcc2^=^buf[i];
}

//ja temos a nossa beautiful frame without byte stuffing
strcat(input, buf);
strcat(input, &bcc);

//Byte Stuffing
//n tenho de ir verificar bit a bit, porque a leitura é feita byte a byte
  stuffed[0]=Flag; int inputSize=(bufSize*%8);
  for(i=1;i<inputSize;i++){
    if((input[i]=='0x7e') || (input[i]=='0x7d')){
      stuffed[i+k]='0x7d';
      k++;
      stuffed[i+k]=input[i]^'0x20';
    }else{
      stuffed[i+k]=input[i];
    }
  }
stuffedSize=i+k;

strcat(stuffed, Flag);
write(app.fileDescriptor, stuffed, stuffedSize);
return 1;
}

*/

int main(){
  llopen(0);
	char buf[40], aux;
	buf[0]=Flag;
	buf[1]=A;
	buf[2]=0x01;
	buf[3]=A^0x01;
	printf("where am I\n");
	strcat(buf, "Hola mina, como estais");
	printf("Is it the minas\n");
	aux=Flag;
	printf("whaaat\n");
	strcat(buf, &aux);
	printf("segmentating\n");	
	write(app.fileDescriptor, buf, 256);
}


  
 

