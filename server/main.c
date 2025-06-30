#include "server.h"
#include "commands.h"
int main()
{
    log_message("INFO", "Server is starting");
    start_server(8080);
    log_message("INFO", "Server has stopped");
    return 0;
}