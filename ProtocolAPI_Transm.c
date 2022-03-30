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

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS10"
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
  struct applicationLayer app;  
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
    if ((strcmp("/dev/ttyS10", layer.port)!=0) && 
  	      (strcmp("/dev/ttyS11", layer.port)!=0)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */



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
  while(1){
  switch(state){

    //escrever informaçao e verificar
    case 0:  

    if(k>layer.numTransmissions){
      printf("Nao recebeste dados nenhuns amigo, problems de envio\n");break;
    }
    k++;
    write(fd,write,5);
    state=1;
    alarm(layer.timeout);    
    continue;

    //receber informaçao
    case 1:  
    
    i=0;
    while (STOP==FALSE) {                            /* loop for input */    
      if (state) {res = read(fd,&output[i],1);}else break;         //lemos só de 1 em 1 para simplificar as coisas, até pq esta função somehow le 8 bytes at once
      if ((i>0)&&(output[i]==Flag)) {alarm(0);STOP=TRUE; printf("Our buff leu isto:%s\n", output); state=2;printf("So conseguiste information back na tua %d transmissao\n", k+1);}
      i++;
    }
    STOP=FALSE;
    
    continue;

  //verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
  case 2:

  if(output[0]==Flag){
    printf("Primeira Flag recebida direito!\n");
  }else state=0;continue;printf("Erros de transmissao, repeat please bro!\n");
  if(output[1]==A){
    printf("Address very well received!\n");
  }else state=0;continue;printf("Erros de transmissao, repeat please bro!\n");
  if(output[2]==UA){
    printf("UA IS HERE WITH US TOOOOO!\n");
  }else state=0;continue;printf("Erros de transmissao, repeat please bro!\n");
  if(output[3]==BCC_R){
    printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
  }else state=0;continue;printf("Erros de transmissao, repeat please bro!\n");
  if(output[4]==Flag){
    printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");break;
  }else state=0;continue;printf("Erros de transmissao, repeat please bro!\n");


  }
  }
}

int main(){
  
}


  
 

