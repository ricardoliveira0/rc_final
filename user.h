typedef struct user User;
#define USER_MAX_VAR_SIZE 10

User * new_user();
bool isClientOnline(char buffer[USER_MAX_VAR_SIZE]);
bool isUserAuth(char nick[USER_MAX_VAR_SIZE], char buffer[USER_MAX_VAR_SIZE]);
bool isUserOper(User * user);
void clientRemove(char nick[USER_MAX_VAR_SIZE]);
void clientReg(char message[2*USER_MAX_VAR_SIZE+1]);
bool setClientOper(char nick[USER_MAX_VAR_SIZE]);
bool removeClientOper(char nick[USER_MAX_VAR_SIZE]);
