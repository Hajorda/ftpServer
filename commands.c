// put <filename> - sends file to server
// get <filename> - gets file from server
// list - lists saved files on server
// wmi - pwd - prints current working directory
// ac <directory> - changes current working directory on server
// del <filename> - removes file from server

// CLIENT COMMANDS
// exit - exits the client
// help - shows this help message

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>


