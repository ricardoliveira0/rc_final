typedef struct user User;
#define USER_MAX_VAR_SIZE 10

User * new_user();
void regs_user(char nick[USER_MAX_VAR_SIZE], char pw[USER_MAX_VAR_SIZE]);
bool isAlreadyRegs(char nick[USER_MAX_VAR_SIZE]);
bool isClientOnline(char buffer[USER_MAX_VAR_SIZE]);
bool isUserAuth(char nick[USER_MAX_VAR_SIZE], char buffer[USER_MAX_VAR_SIZE]);