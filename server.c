#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <time.h>
#include <stdbool.h>
#include "user.c"

#define PORT 5555 
#define CHAR_SIZE 10
#define BUFSIZE 512 


bool hasSpecialChar(char buffer[BUFSIZE]) // function that verify if there is any special char within given char array
{
  char xChar;
  int value;
  bool flag = false;

  for (int k = 0; k < strlen(buffer)-1; k++)
  {
    xChar = buffer[k];
    value = xChar;
    if ((value >= 97 && value <= 122) || (value >= 65 && value <= 90) || (value >= 48 && value <= 57))
    {
      flag  = false; // is alphanumerical
    }
    else
    {
      return true; // is special char
    }
  }
  return flag; 
}

int main(int argc, char const *argv[]) // main
{
  User ** user = malloc(99 * sizeof(User)); // allocates the necessary size for struct User
  fd_set all_fds; // master file descriptor list
  fd_set sel_fds; // temp file descriptor list for select()
  int maxfd; // maximum file descriptor number
  
  int server_fd, new_socket; 
  struct sockaddr_in address;
  
  int opt = 1; // for setsockopt() SO_REUSEADDR, below
  int addrlen = sizeof(address); 
  char buffer[BUFSIZE];
  int bytes;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // creates socket file descriptor 
  { 
    perror("socket failed"); 
    exit(EXIT_FAILURE); 
  }
  
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // forcefully attatches socket to the port 5555
  { 
    perror("setsockopt failed"); 
    exit(EXIT_FAILURE); 
  }

  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons(PORT); 
  
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) // bind the socket to the network address and port
  { 
    perror("bind failed"); 
    exit(EXIT_FAILURE); 
  } 

  if (listen(server_fd, 3) < 0) 
  { 
    perror("listen failed"); 
    exit(EXIT_FAILURE); 
  }

  FD_ZERO(&all_fds); // clear the master and temp sets
  FD_SET(server_fd, &all_fds); // add socket to set of fds




  maxfd = server_fd;
  
  while(true) // server loop
  {
    sel_fds = all_fds; // copy set of fds (because select() changes it)

    if (select(maxfd + 1, &sel_fds, NULL, NULL, NULL) == -1) 
    {
      perror("select failed");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= maxfd; i++) 
    {
      if (FD_ISSET(i, &sel_fds))
      {
        if (i == server_fd)
        {
          if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) 
          { 
            perror("accept failed");
            exit(EXIT_FAILURE);
          }
          printf("Client connected with socket %d.\n", new_socket);
          user[new_socket] = new_user();
          FD_SET(new_socket, &all_fds);
          maxfd = new_socket > maxfd ? new_socket : maxfd;
        }
        else 
        {
          bzero(buffer, BUFSIZE);
          
          bytes = recv(i, buffer, BUFSIZE, 0);
          // command list down below
          if (!strncmp(buffer, "NICK ", 5)) // command NICK <nickname>
          {
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (!strcmp(buffer, "\n") || !strcmp(buffer, "")) // verifies if nick is actually provided
            {
              send(i, "RPLY 002 - Erro. Falta introdução do nome.", BUFSIZE, 0);
            }
            else if (strlen(buffer) > 10 || hasSpecialChar(buffer)) // verifies if nick is within the limits and has any special char
            {
              send(i, "RPLY 003 - Erro. Nome pedido não válido.", BUFSIZE, 0);
            }
            else if (isClientOnline(buffer)) // verifies if nick is already in use
            {
              send(i, "RPLY 004 - Erro. Nome já em uso.", BUFSIZE, 0);
            }
            else
            {
              char oldNick[10], cb[BUFSIZE];
              strcpy(oldNick, user[i] -> nick);
              buffer[bytes] = 0;
              strcpy(user[i] -> nick, buffer);

              if (!strcmp(oldNick, "ANON")) // if nick is still default then is a new user
              {    
                for (int k = 4; k < maxfd + 1; k++)
                {
                  strcpy(cb, "server :> novo utilizador ");
                  strcat(cb, user[i] -> nick);
                  send(k, cb, BUFSIZE, 0);
                }
                       
              } 
              else // otherwise user has already a nick, changing
              {
                oldNick[strlen(oldNick)] = 0;
                for (int k = 4; k < maxfd + 1; k++)
                {
                  strcpy(cb, "server :> ");
                  strcat(cb, oldNick);
                  strcat(cb, " mudou o seu nome para ");
                  strcat(cb, user[i] -> nick);
                  send(k, cb, BUFSIZE, 0);
                }
              }
              buffer[strlen(buffer)] = 0;

              FILE * fp = fopen("online.txt", "a");
              fprintf(fp, "%s\n", buffer);
              fclose(fp);

              send(i, "RPLY 0001 - Nome atribuído com sucesso.",BUFSIZE, 0);

            }
          }
          else if (!strncmp(buffer, "MSSG ", 5)) // command MSSG <message>
          {
            char mssgWithNick[BUFSIZE + 4 + 10]; // declares new char array to send the message with nick 
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix

            if (!strcmp(buffer, "\n") || !strcmp(buffer, "")) // verifies if message is actually provided
            {
              send(i, "RPLY 102 - Erro. Não há texto na mensagem.", BUFSIZE, 0);
            }
            else if (strlen(buffer) > BUFSIZE) // verifies if message is within the limits
            {
              send(i, "RPLY 103 - Erro. Mensagem demasiado longa.", BUFSIZE, 0);
            } 
            else 
            {
              for (int j = 4; j <= maxfd; j++) 
              {
                if (i != j && user[i] -> room == user[j] -> room) // sends message to all other users connected to the same ch
                {
                  strcpy(mssgWithNick, "[");
                  strcat(mssgWithNick, user[i] -> nick);
                  strcat(mssgWithNick, "]: ");
                  strcat(mssgWithNick, buffer);
                  send(j, mssgWithNick, strlen(mssgWithNick), 0 );
                } 
                else if (i == j) // if user is source then send success callback
                {
                  send(i, "RPLY 101 - Mensagem enviada com sucesso.", BUFSIZE, 0);
                }
              }
            }
          }
          else if (!strncmp(buffer, "PASS ", 5)) // command PASS <password>
          {
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
						if (!strcmp(user[i] -> nick, "\n") || !strcmp(user[i] -> nick, "")) // verifies if user nick actually exists
						{
							send(i, "RPLY 202 - Erro. Nome não está definido.", BUFSIZE, 0);
						}
						else if (isUserAuth(user[i] -> nick, buffer)) // verifies if user nick actually exists
						{
              strcpy(user[i] -> pw, buffer);
							send(i, "RPLY 201 - Autenticação com sucesso.", BUFSIZE, 0);
						}
						else // 'isUserAuth' returned false, so pw is not matching the one in the file
						{
							send(i, "RPLY 203 - Erro. Password incorreta.", BUFSIZE, 0);
						}
          }
          else if (!strncmp(buffer, "JOIN ", 5)) // command JOIN <room no>
          {
            int lastRoom = user[i] -> room; // initializes a new var with the current room
            char cb[BUFSIZE];
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (!isUserAuth(user[i] -> nick, user[i] -> pw)) // user is not auth, no previleges to change ch
            {
              send(i, "RPLY 303 - Erro. Não pode mudar para o canal.", BUFSIZE, 0);
            }
            else if (!strncmp(buffer, "0", 1)) // input was 0
            {
              user[i] -> room = 0; // changes the ch
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 0)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k) // successful join callback for already connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k) // successful leave callback for last connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                } 
              }
            }
            else if (!strncmp(buffer, "1", 1)) // input was 1
            {
              user[i] -> room = 1; // changes the ch
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 1)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k) // successful join callback for already connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k) // successful leave callback for last connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                }
              }
            }
            else if (!strncmp(buffer, "2", 1)) // input was 2
            {
              user[i] -> room = 2; // changes the ch
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 2)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k) // successful join callback for already connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k) // successful leave callback for last connected users
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                }
              }
            }
            else // the ch especified does not exists
            {
              send(i, "RPLY 302 - Erro. Canal não existe.", BUFSIZE, 0);
            }
          }
          else if (!strncmp(buffer, "LIST", 4)) // command LIST [no args]
          {
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              char chList[BUFSIZE];
              strcpy(chList, "RPLY 401 0 - default | 1 - cn | 2 - oss");
              send(i, chList, BUFSIZE, 0); // sends callback with the available chs
            }
          }
          else if (!strncmp(buffer, "WHOS", 4)) // command WHOS [no args]
          {
            char cb[BUFSIZE];
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              send(i, "\nLista de utilizadores neste canal:", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room) // verifies if user is connected to the same ch as source
                {
                  
                  strcpy(cb, "nick: ");
                  strcat(cb, user[k] -> nick);
                  strcat(cb, " || oper: ");
                  if (isUserOper(user[k])) 
                  {
                    strcat(cb, "true");
                  }
                  else
                  {
                    strcat(cb, "false");
                  }
                  
                  send(i, cb, BUFSIZE, 0); // send callback with all connected users' information
                }
              }
            }
          }
          else if (!strncmp(buffer, "KICK ", 5)) // command KICK [no args]
          {
            char cb[BUFSIZE];
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              if (isUserOper(user[i]))
              {
                clientRemove(buffer);
                send(i, "RPLY 601 - Utilizador expulso.", BUFSIZE, 0);
                for (int k = 4; k < maxfd + 1; k++)
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, buffer);
                  strcat(cb, " foi expulso.");
                  send(k, cb, BUFSIZE, 0);
                }
              }
              else
              {
                send(i, "RPLY 602 – Erro. Ação não autorizada, utilizador cliente não é um operador.", BUFSIZE, 0);
              }
            }
          }
          else if (!strncmp(buffer, "REGS ", 5)) // command KICK [no args]
          {
            char cb[BUFSIZE], nickReg[CHAR_SIZE];
            bzero(nickReg, CHAR_SIZE);
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              if (isUserOper(user[i]))
              {
                clientReg(buffer);
                for (int i = 0; i < strlen(buffer); i++)
                {
                  if (strncmp(&buffer[i], " ", 1))
                  {
                      strncpy(&nickReg[i], &buffer[i], 1);
                  }
                  else if (!strncmp(&buffer[i], " ", 1)) break;
                }
                
                send(i, "RPLY 701 – Utilizador foi registado com sucesso.", BUFSIZE, 0);
                for (int k = 4; k < maxfd + 1; k++)
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, nickReg);
                  strcat(cb, " foi registado.");
                  send(k, cb, BUFSIZE, 0);
                }
              }
              else
              {
                send(i, "RPLY 702 – Erro. Ação não autorizada, utilizador cliente não é um operador.", BUFSIZE, 0);
              }
            }
          }
          else if (!strncmp(buffer, "OPER ", 5)) // command KICK [no args]
          {
            char cb[BUFSIZE];
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              if (isUserOper(user[i]))
              {
                if (setClientOper(buffer))
                {
                  send(i, "RPLY 801 – Foi promovido a operador.", BUFSIZE, 0);
                  buffer[strlen(buffer)-1] = 0;
                  for (int k = 4; k < maxfd + 1; k++)
                  {
                    strcpy(cb,"server :> ");
                    strcat(cb, buffer);
                    strcat(cb, " foi promovido a operador.");
                    send(k, cb, BUFSIZE, 0);
                  }
                } 
                else
                {
                  buffer[strlen(buffer)-1] = 0;
                  strcpy(cb, "RPLY 804 – Erro. Ação não autorizada, utilizador ");
                  strcat(cb, buffer);
                  strcat(cb, " não é um utilizador registado");
                  send(i, cb, BUFSIZE, 0);
                }
              }
              else
              {
                send(i, "RPLY 802 – Erro. Ação não autorizada, utilizador cliente não é um operador.", BUFSIZE, 0);
              }
            }
            else
            {
              send(i, "RPLY 803 – Erro. Ação não autorizada, utilizador cliente não está autenticado.", BUFSIZE, 0);
            }
          }
          else if (!strncmp(buffer, "QUIT", 4)) // command KICK [no args]
          {
            char cb[BUFSIZE];
            if (isUserAuth(user[i] -> nick, user[i] -> pw)) // verifies if user is auth
            {
              if (isUserOper(user[i]))
              {
                if (removeClientOper(user[i] -> nick))
                {
                  send(i, "RPLY 901 – Deixou de ser operador", BUFSIZE, 0);
                  for (int k = 4; k < maxfd + 1; k++)
                  {
                    strcpy(cb,"server :> ");
                    strcat(cb, user[i] -> nick);
                    strcat(cb, " deixou de ser operador.");
                    send(k, cb, BUFSIZE, 0);
                  }
                } 
              }
              else
              {
                send(i, "RPLY 902 – Erro. Ação não autorizada, utilizador cliente não é um operador.", BUFSIZE, 0);
              }
            }
          }
        }
      }
    }
  }
  return 0; 
} 