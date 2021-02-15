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
#define BUFSIZE 512 


bool hasSpecialChar(char buffer[BUFSIZE]) // function that verify if there is any special char within given char array
{
  char xChar;
  int value;

  for (int k = 0; k < strlen(buffer)-1; k++)
  {
    xChar = buffer[k];
    value = xChar;
    if ((value >= 97 && value <= 122) || (value >= 65 && value <= 90) || (value >= 48 && value <= 57))
    {
      return false; // is alphanumerical
    }
  }
  return true; // is special char
}

int main(int argc, char const *argv[]) // main
{
  User ** user = malloc(99 * sizeof(User)); // allocates the necessary size for struct User
  fd_set all_fds;
  fd_set sel_fds;
  int maxfd;
  
  int server_fd, new_socket; 
  struct sockaddr_in address;
  
  int opt = 1;
  int addrlen = sizeof(address); 
  char buffer[BUFSIZE];
  int bytes;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  { 
    perror("socket failed"); 
    exit(EXIT_FAILURE); 
  }
  
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  { 
    perror("setsockopt failed"); 
    exit(EXIT_FAILURE); 
  }

  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons(PORT); 
  
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
  { 
    perror("bind failed"); 
    exit(EXIT_FAILURE); 
  } 

  if (listen(server_fd, 3) < 0) 
  { 
    perror("listen failed"); 
    exit(EXIT_FAILURE); 
  }

  FD_ZERO(&all_fds);
  FD_SET(server_fd, &all_fds);

  maxfd = server_fd;
  
  while(true) // server loop
  {
    sel_fds = all_fds;

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
            //char nickname[BUFSIZE + 4 + 10];
            memmove(buffer, buffer + 5, BUFSIZE); // removes the command prefix
            if (!strcmp(buffer, "\n") || !strcmp(buffer, "")) // verifies if nick is actually provided
            {
              send(i, "RPLY 002 - Erro: Falta introdução do nome.", BUFSIZE, 0);
            }
            else if (strlen(buffer) > 10 || hasSpecialChar(buffer)) // verifies if nick is within the limits and has any special char
            {
              send(i, "RPLY 003 - Erro: Nome pedido não válido.", BUFSIZE, 0);
            }
            else if (isClientOnline(buffer)) // verifies if nick is already in use
            {
              send(i, "RPLY 004 - Erro: Nome já em uso.", BUFSIZE, 0);
            }
            else
            {
              char oldNick[10];
              strcpy(oldNick, user[i] -> nick);
              buffer[bytes] = 0;
              strcpy(user[i] -> nick, buffer);

              if (!strcmp(oldNick, "ANON")) // if nick is still default then is a new user
              {
                printf("Novo utilizador criado: %s\n", user[i] -> nick);              
              } 
              else // otherwise user has already a nick, changing
              {
                oldNick[strlen(oldNick)] = 0;
                printf("%s mudou o seu nickname para %s\n", oldNick, user[i] -> nick);
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
              send(i, "RPLY 102 - Erro: Não há texto na mensagem.", BUFSIZE, 0);
            }
            else if (strlen(buffer) > BUFSIZE) // verifies if message is within the limits
            {
              send(i, "RPLY 103 - Erro: Mensagem demasiado longa.", BUFSIZE, 0);
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
						else
						{
							send(i, "RPLY 203 - Erro. Password incorreta.", BUFSIZE, 0);
						}
          }
          else if (!strncmp(buffer, "JOIN ", 5))
          {
            int lastRoom = user[i] -> room;
            char cb[BUFSIZE];
            memmove(buffer, buffer + 5, BUFSIZE);
            if (!isUserAuth(user[i] -> nick, user[i] -> pw))
            {
              send(i, "RPLY 303 - Erro. Não pode mudar para o canal.", BUFSIZE, 0);
            }
            else if (!strncmp(buffer, "0", 1))
            {
              user[i] -> room = 0;
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 0)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k)
                {
                  printf("ROOM DO I = %d | ROOM DO K = %d", user[i] -> room, user[k] -> room);
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k)
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                } 
              }
            }
            else if (!strncmp(buffer, "1", 1))
            {
              user[i] -> room = 1;
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 1)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k)
                {
                  printf("ROOM DO I = %d | ROOM DO K = %d", user[i] -> room, user[k] -> room);
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k)
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                }
              }
            }
            else if (!strncmp(buffer, "2", 1))
            {
              user[i] -> room = 2;
              send(i, "RPLY 301 - Mudança de canal com sucesso. (CH 2)", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room && i != k)
                {
                  printf("ROOM DO I = %d | ROOM DO K = %d", user[i] -> room, user[k] -> room);
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " entrou neste canal");
                  send(k, cb, BUFSIZE, 0);
                }
                else if (lastRoom == user[k] -> room && i != k)
                {
                  strcpy(cb,"server :> ");
                  strcat(cb, user[i] -> nick);
                  strcat(cb, " deixou este canal");
                  send(k, cb, BUFSIZE, 0);
                }
              }
            }
            else 
            {
              send(i, "RPLY 302 - Erro. Canal não existe.", BUFSIZE, 0);
            }
          }
          else if (!strncmp(buffer, "LIST", 4))
          {
            if (isUserAuth(user[i] -> nick, user[i] -> pw))
            {
              char chList[BUFSIZE];
              strcpy(chList, "RPLY 401 0 - default | 1 - cn | 2 - oss");
              send(i, chList, BUFSIZE, 0);
            }
          }
          else if (!strncmp(buffer, "WHOS", 4))
          {
            char cb[BUFSIZE];
            if (isUserAuth(user[i] -> nick, user[i] -> pw))
            {
              send(i, "\nLista de utilizadores neste canal:", BUFSIZE, 0);
              for (int k = 4; k < maxfd + 1; k++)
              {
                if (user[i] -> room == user[k] -> room)
                {
                  
                  strcpy(cb, "nick: ");
                  strcat(cb, user[k] -> nick);
                  strcat(cb, " || oper: ");
                  if (user[k] -> oper) 
                  {
                    strcat(cb, "true");
                  }
                  else
                  {
                    strcat(cb, "false");
                  }
                  
                  send(i, cb, BUFSIZE, 0);
                }
              }
            }
          }
        }
      }
    }
  }
  return 0; 
} 