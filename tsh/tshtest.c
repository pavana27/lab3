/*.........................................................................*/
/*                  TSHTEST.C ------> TSH test program                     */
/*                                                                         */
/*                  By N. Isaac Rajkumar [April '93]                       */
/*                  February '13, updated by Justin Y. Shi                 */
/*.........................................................................*/

#include "tshtest.h"
int status;

int main(int argc, char **argv)
{
	static void (*op_func[])() =
	{
		OpPut, OpGet, OpGet, OpExit, OpRshell
  } ;

	if (argc < 2) {
       printf("Usage : %s port\n", argv[0]) ;
       exit(1) ;
  }
	port = atoi(argv[1]);
	while (TRUE) {
       this_op = drawMenu() + TSH_OP_MIN - 1 ;
			 //printf("Operation is %d", this_op);
       if (this_op >= TSH_OP_MIN && this_op <= TSH_OP_MAX) {
			   this_op = htons(this_op) ;
			   tshsock = connectTsh(port) ;
			   // Send this_op to TSH
			   if (!writen(tshsock, (char *)&this_op, sizeof(this_op))) {
				   perror("main::writen\n") ;
				   exit(1) ;
				 }
				//printf("sent tsh op \n");
			   // Response processing
			   (*op_func[ntohs(this_op) - TSH_OP_MIN])() ;
				 //printf("sent tsh op123 \n");
			   close(tshsock) ;
		   } else
	  		return 0 ;
    }
}


void OpPut()
{
   tsh_put_it out ;
   tsh_put_ot in ;
   int tmp ;
   char *buff,*st ;

   status=system("clear") ;
   printf("TSH_OP_PUT") ;
   printf("\n----------\n") ;
				/* obtain tuple name, priority, length, */
   printf("\nEnter tuple name : ") ; /* and the tuple */
   status=scanf("%s", out.name) ;
   printf("Enter priority : ") ;
   status=scanf("%d", &tmp) ;
   out.priority = (u_short)tmp ;
   printf("Enter length : ") ;
   status=scanf("%d", &out.length) ;
   getchar() ;
   printf("Enter tuple : ") ;
   buff = (char *)malloc(out.length) ;
   st=fgets(buff, out.length, stdin) ;
				/* print data sent to TSH */
   printf("\n\nTo TSH :\n") ;
   printf("\nname : %s", out.name) ;
   printf("\npriority : %d", out.priority) ;
   printf("\nlength : %d", out.length) ;
   printf("\ntuple : %s\n", buff) ;

   out.priority = htons(out.priority) ;
   out.length = htonl(out.length) ;
				/* send data to TSH */
   if (!writen(tshsock, (char *)&out, sizeof(out)))
    {
       perror("\nOpPut::writen\n") ;
       getchar() ;
       free(buff) ;
       return ;
    }
				/* send tuple to TSH */
   if (!writen(tshsock, buff, ntohl(out.length)))
    {
       perror("\nOpPut::writen\n") ;
       getchar() ;
       free(buff) ;
       return ;
    }
				/* read result */
   if (!readn(tshsock, (char *)&in, sizeof(in)))
    {
       perror("\nOpPut::readn\n") ;
       getchar() ;
       return ;
    }
				/* print result from TSH */
   printf("\n\nFrom TSH :\n") ;
   printf("\nstatus : %d", ntohs(in.status)) ;
   printf("\nerror : %d\n", ntohs(in.error)) ;
   getchar() ;
}


void OpGet()
{
   tsh_get_it out ;
   tsh_get_ot1 in1 ;
   tsh_get_ot2 in2 ;
   struct in_addr addr ;
   int sd, sock ;
   char *buff ;

   status=system("clear") ;
   printf("TSH_OP_GET") ;
   printf("\n----------\n") ;
				/* obtain tuple name/wild card */
   printf("\nEnter tuple name [wild cards ?, * allowed] : ") ;
   status=scanf("%s", out.expr) ;
				/* obtain port for return data if tuple */
   out.host = gethostid() ;	/* is not available */
   if ((sd = get_socket()) == -1)
    {
       perror("\nOpGet::get_socket\n") ;
       getchar() ; getchar() ;
       return ;
    }
   if (!(out.port = bind_socket(sd, 0)))
    {
       perror("\nOpGet::bind_socket\n") ;
       getchar() ; getchar() ;
       return ;
    }
   addr.s_addr = out.host ;
				/* print data  sent to TSH */
   printf("\n\nTo TSH :\n") ;
   printf("\nexpr : %s", out.expr) ;
   printf("\nhost : %s", inet_ntoa(addr)) ;
   printf("\nport : %d\n", ntohs(out.port)) ;
				/* send data to TSH */
   if (!writen(tshsock, (char *)&out, sizeof(out)))
    {
       perror("\nOpGet::writen\n") ;
       getchar() ; getchar() ;
       close(sd) ;
       return ;
    }
				/* find out if tuple available */
   if (!readn(tshsock, (char *)&in1, sizeof(in1)))
    {
       perror("\nOpGet::readn\n") ;
       getchar() ; getchar() ;
       close(sd) ;
       return ;
    }
				/* print whether tuple available in TSH */
   printf("\n\nFrom TSH :\n") ;
   printf("\nstatus : %d", ntohs(in1.status)) ;
   printf("\nerror : %d\n", ntohs(in1.error)) ;
				/* if tuple is available read from the same */
   if (ntohs(in1.status) == SUCCESS) /* socket */
      sock = tshsock ;
   else				/* get connection in the return port */
      sock = get_connection(sd, NULL) ;
				/* read tuple details from TSH */
   if (!readn(sock, (char *)&in2, sizeof(in2)))
    {
       perror("\nOpGet::readn\n") ;
       getchar() ; getchar() ;
       close(sd) ;
       return ;
    }				/* print tuple details from TSH */
   printf("\nname : %s", in2.name) ;
   printf("\npriority : %d", ntohs(in2.priority)) ;
   printf("\nlength : %d", ntohl(in2.length)) ;
   buff = (char *)malloc(ntohl(in2.length)) ;
				/* read, print  tuple from TSH */
   if (!readn(sock, buff, ntohl(in2.length)))
      perror("\nOpGet::readn\n") ;
   else
      printf("\ntuple : %s\n", buff) ;

   close(sd) ;
   close(sock) ;
   free(buff) ;
   getchar() ; getchar() ;
}


void OpExit()
{
   tsh_exit_ot in ;

   status=system("clear") ;
   printf("TSH_OP_EXIT") ;
   printf("\n-----------\n") ;
				/* read TSH response */
   if (!readn(tshsock, (char *)&in, sizeof(in)))
    {
       perror("\nOpExit::readn\n") ;
       getchar() ;  getchar() ;
       return ;
    }
				/* print TSH response */
   printf("\n\nFrom TSH :\n") ;
   printf("\nstatus : %d", ntohs(in.status)) ;
   printf("\nerror : %d\n", ntohs(in.error)) ;
   getchar() ;  getchar() ;
}


int connectTsh(u_short port)
{
   struct hostent *host ;
   short tsh_port ;
   u_long tsh_host ;
   int sock ;

   // use local host
   tsh_host = inet_addr("127.0.0.1");
   /*
   if ((host = gethostbyname("localhost")) == NULL)
	{
	   perror("connectTsh::gethostbyname\n") ;
	   exit(1) ;
	}
   tsh_host = *((long *)host->h_addr_list[0]) ;
   */
   tsh_port = htons(port);
				/* get socket and connect to TSH */
   if ((sock = get_socket()) == -1)
    {
       perror("connectTsh::get_socket\n") ;
       exit(1) ;
    }
   if (!do_connect(sock, tsh_host, tsh_port))
    {
       perror("connectTsh::do_connect\n") ;
       exit(1) ;
    }
   return sock ;
}


u_short drawMenu()
{
   int choice ;
				/* draw menu of user options */
   status=system("clear") ;
   printf("\n\n\n\t\t\t---------") ;
   printf("\n\t\t\tMAIN MENU") ;
   printf("\n\t\t\t---------") ;
   printf("\n\n\t\t\t 1. Put") ;
   printf("\n\t\t\t 2. Get") ;
   printf("\n\t\t\t 3. Read") ;
   printf("\n\t\t\t 4. Exit (TSH)") ;
	 printf("\n\t\t\t 5. RShell") ;
   printf("\n\t\t\t 6. Quit from this program") ;

   printf("\n\n\n\t\t\tEnter Choice : ") ;

   status=scanf("%d", &choice) ;	/* return user choice */
   return choice ;
}

void OpRshell()
{

	 struct in_addr addr ;
   int sd, sock ;
   int status;
	 char *buffer,st ;
	 int firstTime = 1;

   status=system("clear") ;

	 printf("TSH_OP_RSHELL") ;
   printf("\n----------\n") ;
				/* obtain tuple name/wild card */
   printf("\nWelcome to RShell:>");
	 scanf("%c", &st);
	 do {
		 tsh_rshell_it out ;
	 	 tsh_rshell_ot in ;
	 	 tsh_rshell_ot in1 ;
		 int i=0;
		 char c;
		 //printf("\nSocket : %d\n", tshsock) ;
		 printf("\nEnter command Owner:> ");
		 status = scanf("%[^\n]", out.name);
		 scanf("%c", &st);
		 printf("Enter command length:> ");
		 status = scanf("%d", &out.len);
		 scanf("%c", &st);
		 printf("Enter command :> ");
		 status = scanf("%[^\n]", out.buffer);

		 while (out.buffer[i]) {
		    c=out.buffer[i];
		    out.buffer[i] = tolower(c);
		    i++;
		  }

		 if(strcmp(out.buffer, "exit") == 0) {
			 close(tshsock) ;
			 break;
		 }

		 printf("\nTo Shell");
		 printf("\n------------");
		 printf("\nOwner: %s", out.name);
		 printf("\nlength : %d", out.len);
		 printf("\nEntered command : %s", out.buffer);


	   addr.s_addr = out.host ;

		 //printf("\nSd : %d\n", tshsock) ;
		 //printf("\n\nTo TSH :\n") ;
	   //printf("\ncommand : %s", out.buffer) ;
	   //printf("\nhost : %s", inet_ntoa(addr)) ;
	   //printf("\nport : %d\n", ntohs(out.port)) ;
		 //printf("\nOp : %d\n", this_op) ;

		 if (!writen(tshsock, (char *)&out, sizeof(out))) {
	       perror("\nOpRshell::writen\n") ;
	       getchar() ; getchar() ;
	       close(tshsock) ;
	       return ;
	    }

			if (!readn(tshsock, (char *)&in1, sizeof(in1))) {
	        perror("\nOpRshell::readn\n") ;
	        getchar() ; getchar() ;
	        close(tshsock) ;
	        return ;
	     }

			 //printf("\n\nFrom TSH :\n") ;
		   //printf("%s", in1.buffer) ;
			 getchar();
			 close(tshsock);

			 //this_op = htons(this_op) ;
			 tshsock = connectTsh(port) ;
			 //printf("sent tsh op - end %d\n", this_op);
			 //printf("sent tsh op - end %d\n", tshsock);

			 // Send this_op to TSH
			 if (!writen(tshsock, (char *)&this_op, sizeof(this_op))) {
				 perror("main::writen\n") ;
				 exit(1) ;
			 }

			 //printf("sent tsh op - end \n");
			 //getchar();

	 } while(1);
	 getchar();
}
