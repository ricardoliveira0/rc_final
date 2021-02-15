#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "user.h"
#define USER_MAX_VAR_SIZE 10

struct user
{
    char * nick;
    char * pw;
    bool oper;
    int room;
};

User * new_user()
{
    User * user = malloc(sizeof(User));
    user -> nick = malloc(USER_MAX_VAR_SIZE * sizeof(char));
    user -> pw = malloc(USER_MAX_VAR_SIZE * sizeof(char));
    user -> oper = false; // initializes default oper as false
    user -> room = 0; // initializes default ch as 0

    strcpy(user -> nick, "ANON"); // initializes default nick as 'ANON'
    return user;
}

bool isClientOnline(char buffer[USER_MAX_VAR_SIZE]) // function that verifies if the NICK requested is already in use
{
    buffer[strlen(buffer) - 1] = 0; // removes '\0'
    char currentNick[USER_MAX_VAR_SIZE];
    FILE * fp = fopen("online.txt", "r"); // opens 'online.txt' file, !read mode

    if (fp == NULL) 
    {
        return false;
    }

    while (fgets(currentNick, USER_MAX_VAR_SIZE, fp) != NULL) // get a full line from the file until it ends
    {
        currentNick[strlen(currentNick) - 1] = 0; // removes '\0'
        if (!strcmp(currentNick, buffer)) // if the NICK requested is equal to a online one, returns true (a client with that NICK is online)
        {
            fclose(fp);
            return true;
        }
        
    }
    fclose(fp); // otherwhise that NICK is free
    return false;
}


bool isUserAuth(char nick[USER_MAX_VAR_SIZE], char buffer[USER_MAX_VAR_SIZE]) // function that verifies if a specific user is registered
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(nick)], pwReg[strlen(buffer)];
    int op;

	FILE * fp = fopen("regs.txt", "r"); // opens 'regs.txt' file, !read mode

	if(fp == NULL)
    {
		return false;
    }


    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL) // get a full line from the file until it ends
    {
        for (int i = 0, j = 0,count = 0; i < strlen(allLine); i++) // go through every single char
        {
            if (count == 0) 
            {
                if (strncmp(&allLine[i], ":", 1) || strlen(nick) > i) // if it don't hit ':', it is still in the nickname (because format is 'nick:pw:oper')
                {                                                    // otherwhise as we get a length smaller than i, we ignore this line
                    strncpy(&nickReg[i], &allLine[i], 1);
                /*  printf("char do txt %c\n", allLine[i]);
                    printf("NICK %s\n", nickReg); */
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(nick, nickReg, strlen(nick))) // if it hits a ':', it has just reached the end of the nick
                {                                                                               // as it is just finished and length is not different (as already checked above)
                    count++;                                                                   // compares both nicks (the given one and the other that's written on the file). continue
                }
                if (!strncmp(&allLine[i], ":", 1) && strncmp(nick, nickReg, strlen(nick))) // if it hits a ':', it has just reached the end of the nick
                {                                                                         // as it is just finished and length is not different (as already checked above)
                    break;                                                               // but both nicks are different, break the cycle
                }
            } 
            else if (count == 1)
            {
                if (strncmp(&allLine[i], ":", 1)) // if it don't hit ':', it is still in the password (because format is 'nick:pw:oper' and we already checked nick)
                {
                    strncpy(&pwReg[j], &allLine[i], 1);
                    j++;
                    //printf("PW %s\n", pwReg);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(buffer, pwReg, strlen(pwReg))) // as it reaches the end of the pw, and when compared to the written one it matches
                {                                                                                // the user is now auth
                    fclose(fp);
                    return true;
                }
            }
        }
        // clear both char arrays after every line
        bzero(nickReg, USER_MAX_VAR_SIZE);
        bzero(pwReg, USER_MAX_VAR_SIZE);
    }
    fclose(fp);
	return false; // when reached the EOF, false is returned. not matched user
}

bool isUserOper(User * user)
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(user -> nick)];
    FILE * fp = fopen("regs.txt", "r"); // opens 'regs.txt' file, !read mode

	if(fp == NULL)
    {
		return false;
    }

    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL)
    {
        for (int i = 0,count = 0; i < strlen(allLine); i++)
        {
            if (count == 0) 
            {
                if (strncmp(&allLine[i], ":", 1) || strlen(user -> nick) > i)
                {
                    strncpy(&nickReg[i], &allLine[i], 1);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(user -> nick, nickReg, strlen(user -> nick)))
                {
                    count++;
                }
                if (!strncmp(&allLine[i], ":", 1) && strncmp(user -> nick, nickReg, strlen(user -> nick)))
                {
                    break;
                }
            } 
            else if (count == 1)
            {
                if (strncmp(&allLine[i], ":", 1))
                {
                    continue;
                } 
                else if (!strncmp(&allLine[i], ":", 1))
                {
                    count++;
                }
            }
            else if (count == 2)
            {
                if (strncmp(&allLine[i], "0", 1))
                {
                    user -> oper = true;
                    fclose(fp);
                    return true;
                } 
                else
                {
                    fclose(fp);
                    return false;    
                } 
            }
        }
        bzero(nickReg, USER_MAX_VAR_SIZE);
    }
    fclose(fp);
	return false; 
}

void clientRemove(char nick[USER_MAX_VAR_SIZE])
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(nick)];
    FILE * fp = fopen("regs.txt", "r"); // opens 'regs.txt' file, !read mode
    FILE * fauxp = fopen("temp.txt", "a");
    nick[strlen(nick) - 1] = 0;

	if(fp == NULL || fauxp == NULL)
    {
		return;
    }

    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL)
    {
        for (int i = 0; i < strlen(allLine); i++)
        {
            if (strncmp(&allLine[i], ":", 1) || strlen(nick) > i)
            {
                strncpy(&nickReg[i], &allLine[i], 1);
            } 
            else if (!strncmp(&allLine[i], ":", 1) && strncmp(nick, nickReg, strlen(nick)))
            {
                fprintf(fauxp, "%s", allLine);
            }
            if (!strncmp(&allLine[i], ":", 1) && !strncmp(nick, nickReg, strlen(nick)))
            {
                break;
            }
        }
        // clear both char arrays after every line
        bzero(nickReg, USER_MAX_VAR_SIZE);
        bzero(allLine, USER_MAX_VAR_SIZE);
    }
    fclose(fp);
    fclose(fauxp);
    remove("regs.txt");
    rename("temp.txt", "regs.txt");
}

void clientReg(char message[2*USER_MAX_VAR_SIZE+1])
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[USER_MAX_VAR_SIZE], pwReg[USER_MAX_VAR_SIZE];
    FILE * fp = fopen("regs.txt", "a"); // opens 'regs.txt' file, !read mode

    bzero(nickReg, USER_MAX_VAR_SIZE);
    bzero(pwReg, USER_MAX_VAR_SIZE);

    for (int i = 0, j = 0, count = 0; i < strlen(message); i++)
    {
        if (count == 0)
        {
            if (strncmp(&message[i], " ", 1))
            {
                strncpy(&nickReg[i], &message[i], 1);
            }
            else if (!strncmp(&message[i], " ", 1)) count ++;
        }
        else if (count == 1)
        {
            if (strncmp(&message[i], "\n", 1))
            {
                strncpy(&pwReg[j], &message[i], 1);
                j++;
            }
            else
            {
                fprintf(fp, "%s:%s:0\n", nickReg, pwReg);
                fclose(fp);
                return;
            }
        }
    }
}

bool setClientOper(char nick[USER_MAX_VAR_SIZE])
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(nick)];
    bool flag = false;
    FILE * fp = fopen("regs.txt", "r"); // opens 'regs.txt' file, !read mode
    FILE * fauxp = fopen("temp.txt", "a");

    bzero(nickReg, USER_MAX_VAR_SIZE);

    if(fp == NULL || fauxp == NULL)
    {
		return false;
    }

    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL) 
    {
        for (int i = 0,count = 0; i < strlen(allLine); i++) // go through every single char
        {
            if (count == 0) 
            {
                if (strncmp(&allLine[i], ":", 1)) 
                {                                                  
                    strncpy(&nickReg[i], &allLine[i], 1);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(nick, nickReg, strlen(nickReg))) 
                {                                                                               
                    count++;                                                                   
                }
                if (!strncmp(&allLine[i], ":", 1) && strncmp(nick, nickReg, strlen(nickReg))) 
                {                                                                                        
                    fprintf(fauxp, "%s", allLine);                                                      
                    break;                                                                               
                }
            } 
            else if (count == 1)
            {
                if (strncmp(&allLine[i], ":", 1)) 
                {
                    continue;
                } 
                else if (!strncmp(&allLine[i], ":", 1)) 
                {                                                                                
                    count++;
                }
            }
            else if (count == 2)
            {
                if (!strncmp(&allLine[i], "0", 1))
                {
                    strncpy(&allLine[i], "1", 1);
                    fprintf(fauxp, "%s", allLine);
                    flag = true;
                    break;
                } 
                else
                {
                    break;  
                } 
            }
        }
        // clear both char arrays after every line
        bzero(nickReg, USER_MAX_VAR_SIZE);
        bzero(allLine, USER_MAX_VAR_SIZE);
    }
    fclose(fp);
    fclose(fauxp);
    remove("regs.txt");
    rename("temp.txt", "regs.txt");
    return flag;
}

bool removeClientOper(char nick[USER_MAX_VAR_SIZE])
{
    char allLine[2*USER_MAX_VAR_SIZE+3], nickReg[strlen(nick)];
    bool flag = false;
    FILE * fp = fopen("regs.txt", "r"); // opens 'regs.txt' file, !read mode
    FILE * fauxp = fopen("temp.txt", "a");

    bzero(nickReg, USER_MAX_VAR_SIZE);

    if(fp == NULL || fauxp == NULL)
    {
		return false;
    }

    while(fgets(allLine, 2*USER_MAX_VAR_SIZE+3, fp) != NULL) 
    {
        for (int i = 0,count = 0; i < strlen(allLine); i++) // go through every single char
        {
            if (count == 0) 
            {
                if (strncmp(&allLine[i], ":", 1)) 
                {                                                  
                    strncpy(&nickReg[i], &allLine[i], 1);
                } 
                else if (!strncmp(&allLine[i], ":", 1) && !strncmp(nick, nickReg, strlen(nickReg))) 
                {                                                                               
                    count++;                                                                   
                }
                if (!strncmp(&allLine[i], ":", 1) && strncmp(nick, nickReg, strlen(nickReg))) 
                {                                                                                        
                    fprintf(fauxp, "%s", allLine);                                                      
                    break;                                                                               
                }
            } 
            else if (count == 1)
            {
                if (strncmp(&allLine[i], ":", 1)) 
                {
                    continue;
                } 
                else if (!strncmp(&allLine[i], ":", 1)) 
                {                                                                                
                    count++;
                }
            }
            else if (count == 2)
            {
                if (!strncmp(&allLine[i], "1", 1))
                {
                    strncpy(&allLine[i], "0", 1);
                    fprintf(fauxp, "%s", allLine);
                    flag = true;
                    break;
                } 
                else
                {
                    break;  
                } 
            }
        }
        // clear both char arrays after every line
        bzero(nickReg, USER_MAX_VAR_SIZE);
        bzero(allLine, USER_MAX_VAR_SIZE);
    }
    fclose(fp);
    fclose(fauxp);
    remove("regs.txt");
    rename("temp.txt", "regs.txt");
    return flag;
}

// WIP

/* void regs_user(char nick[USER_MAX_VAR_SIZE], char pw[USER_MAX_VAR_SIZE])
{
    if(!isAlreadyRegs(nick))
    {
        FILE * fp = fopen("regs.txt", "a");
        fprintf(fp, "%s:%s:%d", nick,pw,0);
    } else {
        printf("Esse nickname já se encontra em uso.");
    }
} */