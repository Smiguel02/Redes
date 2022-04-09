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

/***********************
 * Struct Linklayer received values:
 * ll.serialPort="/dev/ttySx"
 * ll.role=0 se tx, 1 se rx
 * ll.baudRate=9600
 * ll.numTries=3
 * ll.timeOut=3
 * ****************************/

// needs to be global variable, but honestly tenho de ver outra maneira que a stora disse que dava

linkLayer ll;

int fd;
int state = 0;
int S;
int R;

void abertura()
{
  printf("Timedout llopen bro\n");
  state = 0;
}

void escrita(){
  printf("Data not received back on llwrite broooo\n");
}

// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters)
{

  struct termios oldtio, newtio;

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

  (void)signal(SIGALRM, abertura);

    input[0] = FLAG;
    input[1] = A_T;
    input[2] = SET;
    input[3] = BBC1_T;
    input[4] = FLAG;

    while (k <= connectionParameters.numTries && state != 6)
    {
      switch (state)
      {

      // escrever informaçao e verificar
      case 0:
        alarm(0); // reinicia alarm
        printf("Estamos em state=0 pela %d vez\n", k);

        if (k > connectionParameters.numTries)
        {
          printf("Nao recebeste dados nenhuns amigo, problems de envio\n");
          break;
        }

        k++;
        res = write(fd, input, 5);
        printf("%d Bytes written\n", res);
        state = 1;
        alarm(connectionParameters.timeOut);
        break;

      // receber informaçao
      case 1:
        printf("Entramos na 1º funçao read\n");
        while ((!read(fd, &output[0], 1)) && state)
        {
        }
        printf("saimos do primeiro read\n");
        if (output[0] == FLAG)
        {
          printf("Lemos a prImeira flag :)\n");
          state = 2;
          alarm(0); // paramos o timer, porque de facto temos uma receçao de dados
        }
        else
        {
          state = 0;
        }

        break;

      case 2:
        while (!read(fd, &output[1], 1))
        {
        }
        if (output[1] == A_T)
        {
          printf("Address very well received!\n");
          state = 3;
        }
        else
        {
          state = 0;
        }

        break;

      case 3:
        while (!read(fd, &output[2], 1))
        {
        }
        if (output[2] == UA)
        {
          printf("UA IS HERE WITH US TOOOOO!\n");
          state = 4;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;
      case 4:
        while (!read(fd, &output[3], 1))
        {
        }
        if (output[3] == BCC1_R)
        {
          printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
          state = 5;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }
        break;
      case 5:
        while (!read(fd, &output[4], 1))
        {
        }
        if (output[4] == FLAG)
        {
          printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");
          state = 6;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;
      }
    }
    printf("WE DID IT MOTHERFUCKERS\n");

    return 1;

  case RECEIVER:

    input[0] = FLAG;
    input[1] = A_T;
    input[2] = UA;
    input[3] = BCC1_R;
    input[4] = FLAG;

    printf("antes switch \n");

    // control field definition
    while (state != 6)
    {
      // Receiver
      switch (state)
      {
      case 0:
        printf("ENtramos na funçao read\n");
        while (!res)
        {
          res = read(fd, &output[0], 1);
        }

        printf("%d Bytes Read\n", res);

        if (output[0] == FLAG)
        {
          printf("Lemos a primeira flag :)\n");
          state = 1;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;

      case 1:
        res = 0;
        printf("Entramos em state=1\n");
        while (!res)
        {
          res = read(fd, &output[1], 1);
        }
        printf("%d Bytes Read\n", res);
        if (output[1] == A_T)
        {
          printf("Address very well received!\n");
          state = 2;
        }
        else
        {
          state = 0;
          printf("N recebemos o Address\n");
        }

        break;

      case 2:
        res = 0;
        while (!res)
        {
          res = read(fd, &output[2], 1);
        }
        if (output[2] == SET)
        {
          printf("SET IS HERE WITH US TOOOOO!\n");
          state = 3;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;

      case 3:
        res = 0;
        while (!res)
        {
          res = read(fd, &output[3], 1);
        }
        if (output[3] == BBC1_T)
        {
          printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
          state = 4;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;

      case 4:
        res = 0;
        while (!res)
        {
          res = read(fd, &output[4], 1);
        }
        if (output[4] == FLAG)
        {
          printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");
          state = 5;
          break;
        }
        else
        {
          state = 0;
          printf("Erros de transmissao, repeat please bro!\n");
        }

        break;

      case 5:
        // FAZER RETRANS
        res = write(fd, input, 5);
        printf("%d Bytes written\n", res);
        state = 6;
        break;
      }
    }
    printf("Finished Receiver \n");

    return 1;
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

  (void)signal(SIGALRM, escrita);


  input[0] = FLAG;
  input[1] = A_T;
  input[2] = S;   //atualizar S se receçao da resposta foi bem sucedida
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
    if ((input[i] == '0x7e') || (input[i] == '0x7d'))
    {
      stuffed[i + k] = '0x7d';
      k++;
      stuffed[i + k] = input[i] ^ '0x20';
    }
    else
    {
      stuffed[i + k] = input[i];
    }
  }
  stuffedSize = i + k + 1;

  strcat(stuffed, FLAG);
  write(fd, stuffed, stuffedSize);

  //Escrita successful, agora vamos esperar mensagem de resposta

  alarm(ll.timeOut);

  while(!read(fd, input, 5)){}


  return 1;
}

///////////////////////////////////////
//////////////////////////////////////

// Receive data in packet
int llread(char *packet)
{
  int i, fd, length = 0;
  char Read_buf[MAX_PAYLOAD_SIZE * 2], new_buf[2 * MAX_PAYLOAD_SIZE];

  while (length < 0)
  {
    length = read(fd, Read_buf, 1);
  }

  // printf("packet lenght is %d \n", length);
  //char *new_buf = malloc(sizeof(char) * (length));
  char *stuffed = malloc(sizeof(char) * (length * 2 + 8));

  new_buf[0] = FLAG;
  new_buf[1] = A_T;
  new_buf[2] = UA;
  new_buf[3] = BCC1_R;
  new_buf[4] = FLAG;

  while (1)
  {
    i = 0;
    (void)read(fd, &Read_buf[i], 1);

    if ((Read_buf[i] == FLAG) && (i > 0))
    {
      break;
    }
    i++;
    printf("Entrou\n");
  }

  int k = 0;
  for (int j = 0; j < i; j++)
  {

    if (Read_buf[j] == 0x7d)
    {
      if (Read_buf[j + 1] == 0x05e)
      {
        new_buf[k] = 0x7e;
        k++;
        printf("Leu 1 \n");
        continue;
      }
      else if (Read_buf[j + 1] == 0x05d)
      {
        new_buf[k] = Read_buf[j];
        k++;
        printf("Leu 2 \n");
        continue;
      }
    }
    new_buf[k] = Read_buf[j];
    printf("ta \n");
    k++;
  }

  if (i == length)
  {
    printf("Nao recebmos flags, tentar outra vez\n");
    return 0;
  }

  // Byte Stuffing
  // n tenho de ir verificar bit a bit, porque a leitura é feita byte a byte


  stuffed[0] = FLAG;
  int stuffedSize;
  int inputSize = (MAX_PAYLOAD_SIZE * 2);

  for (i = 4; i < inputSize; i++)
  {
    if ((new_buf[i] == '0x7d'))
    {
      if (new_buf[i + 1] == '0x5e')
      {
        // alterar para 0x7e
        stuffed[i + k] = '0x7e';
      }
      else
      { 
        stuffed[i + k] = '0x7d';
        k++;
        stuffed[i + k] = new_buf[i] ^ '0x20';
      }
    }
    else
    {
      stuffed[i + k] = new_buf[i];
    }
    i++;
  }
  stuffedSize = i + k;

  strcat(stuffed, FLAG);

(void)write(fd, new_buf, 5);


}



// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(int showStatistics);