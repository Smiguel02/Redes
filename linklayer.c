#include "linklayer.h"

// Define Flags, Header and Trailer
#define FLAG 0x07
#define A_T 0x03
#define A_R 0x01
#define SET 0x03
#define DISC 0x0b
#define UA 0x07
#define BBC1_T A_T ^ SET // Será usado o mesmo Address no llopen, tanto para Transmitter como Receiver
#define BCC1_R A_T ^ UA

#define HEADER_SIZE 4
#define SUP_SIZE 5

/***********************
 * Struct Linklayer received values:
 * ll.serialPort="/dev/ttySx"
 * ll.role=0 se tx, 1 se rx
 * ll.baudRate=9600
 * ll.numTries=3
 * ll.timeOut=3
 * ****************************/

// needs to be global variable, but honestly tenho de ver outra maneira que a stora disse que dava

struct statistacs{
  int DataWritten;
};


linkLayer ll;
struct termios oldtio, newtio;
int fd;
int state = 0;
int S;
int R;

void escrita(){
  printf("Data not received back on llwrite broooo\n");
  state=0;
}

//control and expected are very well defined already by the asking fucntion
//works only for answers
//returns control variable
char wait_for_answer(){

  int k=0;
  char output[HEADER_SIZE +1], input[HEADER_SIZE+1];

  (void)signal(SIGALRM, escrita);

	if(state=0){
	state=1;
	alarm(0);
	alarm(ll.timeOut);
	}

	while (state <3)
	{
	  switch (state)
	  {

	  case 0:
		return -1;

	  // receber informaçao
	  case 1:
		printf("À espera da resposta\n");
		while ((!read(fd, &input[0], 1)) && state)
		{
		}
		if (input[0] == FLAG)
		{
		  state = 2;
		  alarm(0); // paramos o timer, porque de facto temos uma receçao de dados
		}
		else
		{
		  state = 0;
		}

		break;

	  case 2:
		while (!read(fd, &input[1], 3)){}   //might not work por causa do endereço, ams hard doubt

		if(input[3]!=(input[1]^input[2])){
		  printf("ERRO, bcc1 diferente, retransmite");
		  state=0;
		  break;
		}
		state++;
		break;
		
	  }
	}


	printf("Recebeste um Header quite successfully\n");
	return input[2];
}


// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters)
{
	char aux;

  strcpy(connectionParameters.serialPort,ll.serialPort);
  ll.role=connectionParameters.role;
  ll.baudRate=connectionParameters.baudRate;
  ll.numTries=connectionParameters.numTries;
  ll.timeOut=connectionParameters.timeOut;



  /*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
	perror(connectionParameters.serialPort);
	exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
	perror("tcgetattr");
	exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

  /*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
	perror("tcsetattr");
	exit(-1);
  }

  printf("New termios structure set\n");
  // Comunicaçao aberta

  int k = 0, res = 0;
  char input[5], output[5];

  // activates timeOut interrupt

  switch (connectionParameters.role)
  {

  case TRANSMITTER:

	input[0] = FLAG;
	input[1] = A_T;
	input[2] = SET;
	input[3] = BBC1_T;
	input[4] = FLAG;


	while (k <= connectionParameters.numTries && state != 2)
	{
	
	  switch (state)
	  {

	  // escrever informaçao e verificar
	  case 0:
		printf("Estamos em state=0 pela %d vez\n", k);

		if (k > connectionParameters.numTries)
		{
		  printf("Nao recebeste dados nenhuns amigo, problems de envio\n");
		  break;
		}

		k++;
		res = write(fd, input, 5);
		printf("%d Bytes written\n", res);
		aux=wait_for_answer();
		if(aux!=UA){
			printf("Recebemos ccontrol errado, trying again\n");
			state=0;
			break;
		}

		state=1;
		break;

	  // receber informaçao
		case 1:
			while(!read(fd, &aux, 1)){}

			if(aux!=FLAG){
				printf("Nao recebemos last FLAG, trying again\n");
				state=0;
				break;
			}

			state++;
			printf("llopen Transmitter done successfully (palmas)\n");
			return 1;

		break;
	  
	}

	}
	return 1;
	
	
//Ver return desta function quando ha erro, should I even count the tries??? 
//HELPPPPPPPPP
  case RECEIVER:

	input[0] = FLAG;
	input[1] = A_T;
	input[2] = UA;
	input[3] = BCC1_R;
	input[4] = FLAG;

	printf("A começar RECEIVER\n");

	// control field definition
	while (state != 2)
	{
	  // Receiver
	  switch (state)
	  {
		case 0:
			state=1;
			aux=wait_for_answer();
			if(aux==SET){
				printf("Header well received\n");
				state=1;
				break;
			}

			printf("Header badly received\n");
			state=0;//ou return 0, who knows honestly
			break;
			
		case 1:
			while(!read(fd, &aux, 1)){}

			if(aux!=FLAG){
				printf("Nao recebemos last FLAG\n");
				state=0;	//ou return, who knows
				break;
			}

			state++;
			printf("llopen RECEIVER done successfully (palmas)\n");
			return 1;

		break;


	  }
}
}
}


int llwrite(char *buf, int bufSize)
{

  int i = 0, aux, k = 0, inputSize=bufSize+5;
  int stuffedSize;
  char *input = malloc(sizeof(char) * (inputSize));
  char *stuffed = malloc(sizeof(char) * (bufSize * 2 + 8));
  char *output = malloc(sizeof(char)*5);
  char bcc2 = 0;
  char help;



  input[0] = FLAG;
  input[1] = A_T;
  input[2] = (S<<1);   //atualizar S se receçao da resposta foi bem sucedida
  input[3] = input[1] ^ input[2];




  // BCC2 creation
  // to be honest n sei de onde vem esta ideia do XOR, so we must ask teacher
  for (i = 0; i < bufSize; i++)
  {
	bcc2 ^= buf[i];
  }


  // ja temos a nossa beautiful frame without byte stuffing
  strcat(input, buf);
  strcat(input, &bcc2);   //n vamos adicionar a ultima flag para no stuffing nao a considerar


  // Byte Stuffing
  // n tenho de ir verificar bit a bit, porque a leitura é feita byte a byte
  stuffed[0] = FLAG;
  for (i = 1; i < inputSize; i++)
  {
	if ((input[i] == 0x7e) || (input[i] == 0x7d))
	{
	  stuffed[i + k] = 0x7d;
	  k++;
	  stuffed[i + k] = input[i] ^ 0x20;
	}
	else
	{
	  stuffed[i + k] = input[i];
	}
  }
  stuffedSize = i + k + 1;

  strcat(stuffed, FLAG);



  state=0;
  k=0;
  while(state<2){

	switch (state){
	  
	  case 0:

		if (k > ll.numTries)
		{
		  printf("Nao recebeste dados nenhuns amigo, problems de envio\n");
		  return -1;
		  break;
		}

		k++;

		write(fd, stuffed, stuffedSize);
		help=wait_for_answer();
		if(!help){
			printf("TimedOut, ou mal recebida, sending again\n");
			state=0;
			break;
		}

		state=1;
	  break;

	  case 1:

	  	//add code to verify rejection or not
		while(!read(fd, &help, 1)){}

			if(help!=FLAG){
				printf("Nao recebemos last FLAG\n");
				state=0;	//ou return, who knows
				break;
			}

			state++;
			printf("llwrite done successfully (palmas)(yet again)\n");
			return 1;


	  break;
		}

	  }
	

  }
  


///////////////////////////////////////
//////////////////////////////////////

// Receive data in packet
int llread(char *packet)
{
  int i, fd, length = 0, new_bufSize=0;
  char Read_buf[MAX_PAYLOAD_SIZE * 2], new_buf[HEADER_SIZE + MAX_PAYLOAD_SIZE];
  char output[5];
  char bcc2=0;


  char *stuffed = malloc(sizeof(char) * (length * 2 + 8));

  output[0] = FLAG;
  output[1] = A_T;
  output[2] = UA;
  output[3] = output[1]^output[2];
  output[4] = FLAG;

  
  i=0;
  for(i=0;i<4;i++){
	(void)read(fd, &Read_buf[i], 1);
	if(Read_buf[0]!=FLAG){
	  return 0;
	}
  }

  if(Read_buf[3]!=(Read_buf[1]^Read_buf[2])){
	return 0;
  }

  while (1)
  {
	(void)read(fd, &Read_buf[i], 1);

	if ((Read_buf[i] == FLAG) )
	{
	  i++;
	  break;
	}
	i++;
  }

  //i=size of vector
  int k = -1;
  int j=0;

	while(Read_buf[k+HEADER_SIZE]!=FLAG)
	{
	  if(Read_buf[k+HEADER_SIZE]==0x7d){
		j++;
		k++;
		new_buf[k-j]=Read_buf[k+HEADER_SIZE]^0x20;
	  }else
	  {
		new_buf[k-j]=Read_buf[k+HEADER_SIZE];
	  }
	  bcc2^=new_buf[k-j];
	  k++;
	}

  new_bufSize=k-2;
  
  if(bcc2){
	printf("Error in received data, trying again\n");
	return 0;
  }


(void)write(fd, output, 5);

return new_bufSize;

}



// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(int showStatistics){

}