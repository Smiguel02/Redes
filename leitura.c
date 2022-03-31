    case 1:  
		printf("ENtramos na funçao read\n");
		while(state){
		(void)read(fd,&output[0],1);
	}
		printf("saimos do primeiro read\n");
		if(output[0]==Flag){
			printf("Lemos a promeira flag\n");
			state=2;
			}else state=0;
    
    
break;
  //verificação da mensagem recebida, ja paramos o timer de rececao, porque somos fixes e recebemos tudo
  case 2:
  while(state){
  		(void)read(fd,&output[1],1);}
  if(output[1]==A){
	 printf("Address very well received!\n");
	  }else state=0;
	  
break;	  
	  
	  case 3:
while(state){
		(void)read(fd,&output[2],1);}
	if(output[2]==UA){
    printf("UA IS HERE WITH US TOOOOO!\n");
}else state=0;
    
    
break;    
    case 4: 
    while(state){
    (void)read(fd,&output[3],1);}
      if(output[3]==BCC_R){
    printf("What, os bits de verificaçao(BCC) tambem estao bem? Crazyy\n");
  }else state=0;printf("Erros de transmissao, repeat please bro!\n");
    
break;    
    case 5:
    while(state){
    (void)read(fd,&output[4],1);}
    if(output[4]==Flag){
    printf("RECEBEMOS TUDO EM CONDIÇOES, CONGRATULATIONS\n");break;
  }else state=0;printf("Erros de transmissao, repeat please bro!\n");
    
    break;
    
  

  }
