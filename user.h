typedef struct user User;
#define USER_MAX_VAR_SIZE 10

User * new_user();
bool isClientOnline(char buffer[USER_MAX_VAR_SIZE]);
bool isUserAuth(char nick[USER_MAX_VAR_SIZE], char buffer[USER_MAX_VAR_SIZE])