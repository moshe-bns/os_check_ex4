#include "whatsappio.h"
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <iostream>
#include <map>
#include <algorithm>
#include <vector>


#define MESGSIZE 1700
#define MAX_CLIENTS 500

#define CONSUC "1"
#define CONEXIST "2"
#define CONFAIL "3"
#define SERVFAIL "4"
#define WHOMES "P46R"
#define SENDMES "Y26F"
#define SENDMESFAIL "WER8"
#define GETMES "SD4G"
#define GROUPFAIL "D0FL"
#define GROUPSUC "DS34"
#define CLIENTFAIL "AS5E"
#define MAXHOSTNAME 30


bool check_name(std::string name){
    for(unsigned int i=0; i<name.size (); i++){
        if(!isalnum(name[i])){
            return false; // bad name
        }
    }
    return true; // good name
}

int read_data(int s, char *buf) {
    bzero (buf, MESGSIZE);
    size_t bcount;       /* counts bytes read */
    ssize_t br;               /* bytes read this pass */
    bcount= 0; br= 0;
    while (bcount < MESGSIZE) { /* loop until full buffer */
        br = read(s, buf, MESGSIZE-bcount);
        if (br > 0)
        {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            print_error ("read", errno);
            return -1;
        }
    }
    return((int)bcount);
}


int write_data(int s, char *buf) {
    char send_buf[MESGSIZE];
    bzero (send_buf, MESGSIZE);
    strcpy (send_buf, buf);
    int bcount;       /* counts bytes read */
    int br;               /* bytes read this pass */
    bcount= 0; br= 0;
    while (bcount < MESGSIZE) { /* loop until full buffer */
        br = write(s, send_buf, MESGSIZE-bcount);
        if (br > 0)
        {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            print_error ("write", errno);
            return -1;
        }
    }
    return(bcount);
}

void exit_client(int my_fd){
    char _mes[MESGSIZE];
    bzero (_mes, MESGSIZE);
    strcpy (_mes,CLIENTFAIL);
    if(write_data(my_fd, _mes)<0){
        exit_client (my_fd);
        exit(1);
    }
}

void cotact_server(int my_fd, char* my_name_char)
{
    std::string my_name(my_name_char);
    fd_set set_fd;
    FD_ZERO ( &set_fd );
    FD_SET ( STDIN_FILENO , &set_fd );
    FD_SET ( my_fd , &set_fd );
    while (true)
    {
        fd_set readfds = set_fd;
        if ( select ( MAX_CLIENTS + 1 , &readfds , NULL , NULL , NULL ) < 0 )
        {
            exit_client (my_fd);
            print_error ( "select" , errno);
            exit ( 1 );
        }
        if ( FD_ISSET( my_fd , &readfds ))
        {
            char buf[MESGSIZE];
            if(read_data( my_fd , buf)<0){
                exit_client (my_fd);
                exit(1);
            }
            std::string got(buf);
            if ( strcmp ( buf , CONEXIST ) == 0 )
            {
                print_dup_connection ();
                exit ( 1 );
            }
            if ( strcmp ( buf , CONSUC ) == 0 )
            {
                print_connection ();
            }
            if ( strcmp ( buf , SERVFAIL ) == 0 )
            {
                exit ( 1 );
            }
            if(got.substr (0,4).compare (WHOMES)==0){
                std::string mes_from_serv = got.substr (4,got.size ());
                std::vector<std::string> clients_from_server;
                std::string delimiter = ",";
                size_t pos = 0;
                std::string token;
                while ((pos = mes_from_serv.find(delimiter)) != std::string::npos) {
                    token = mes_from_serv.substr(0, pos);
                    clients_from_server.push_back (token);
                    mes_from_serv.erase(0, pos + delimiter.length());
                }
                clients_from_server.push_back (mes_from_serv);
                print_who_client (true,clients_from_server);
            }
            if(got.substr (0,4).compare (SENDMESFAIL)==0){
                print_send ( false, false,"a", "a","a");
            }
            if(got.substr (0,4).compare (SENDMES)==0){
                print_send ( false, true,"a", "a","a");
            }
            if(got.substr (0,4).compare (GROUPFAIL)==0|| got.substr (0,4).compare (GROUPSUC)==0){
                std::string mes_from_serv = got.substr (4,got.size ());
                print_create_group ( false, got.substr (0,4).compare (GROUPSUC)==0, my_name, mes_from_serv);
            }

            if(got.substr (0,4).compare (GETMES)==0){
                std::string mes_from_serv = got.substr (4,got.size ());
                std::string delimiter = ":";
                size_t pos = 0;
                std::string sender_name;
                if ((pos = mes_from_serv.find(delimiter)) != std::string::npos) {
                    sender_name = mes_from_serv.substr(0, pos);
                    mes_from_serv.erase(0, pos + delimiter.length());
                }
                print_message ( sender_name, mes_from_serv);
            }

        }
        if (FD_ISSET( STDIN_FILENO , &readfds ))
        {
            char buf[MESGSIZE];
            bzero (buf,MESGSIZE);
            std::string mes_std_in;
            std::getline (std::cin,mes_std_in);
            if(mes_std_in.empty ()){
                print_invalid_input ();
                continue;
            }
            strcpy (buf,mes_std_in.c_str ());
            std::string command = strtok(buf, "\r\n");
            command_type commandT;
            std::string name;
            std::string message;
            std::vector<std::string> clients;
            parse_command (command,commandT,name,message,clients);

            //// ---------------------------------------------------------------------------------------------------------------
            bool bad_input=false;
            if(!check_name(name)){
                bad_input=true;
            }
            if(!clients.empty ()){
                for (auto it=clients.begin(); it!=clients.end(); it++){
                    if(!check_name (*it)){
                        bad_input=true;
                    }
                }
            }
            if(bad_input){
                commandT = INVALID;
            }
            //// ---------------------------------------------------------------------------------------------------------------
            switch (commandT){
                case INVALID: print_invalid_input (); break;
                case EXIT: {
                    char _mes[MESGSIZE];
                    bzero (_mes, MESGSIZE);
                    strcpy (_mes,command.c_str ());
                    if(write_data(my_fd, _mes)<0){
                        exit_client (my_fd);
                        exit(1);
                    }
                    print_exit (false, "a");
                    exit(0);
                }
                case WHO:{
                    char _mes[MESGSIZE];
                    bzero (_mes, MESGSIZE);
                    strcpy (_mes,command.c_str ());
                    std::string x(_mes);
                    if(write_data(my_fd, _mes)<0){
                        exit_client (my_fd);
                        exit(1);
                    }
                    break;
                }
                case SEND:{
                    char _mes[MESGSIZE];
                    bzero (_mes, MESGSIZE);
                    strcpy (_mes,command.c_str ());
                    if(write_data(my_fd, _mes)<0){
                        exit_client (my_fd);
                        exit(1);
                    }
                    break;
                }
                case CREATE_GROUP:{
                    char _mes[MESGSIZE];
                    bzero (_mes, MESGSIZE);
                    strcpy (_mes,command.c_str ());
                    if(write_data(my_fd, _mes)<0){
                        exit_client (my_fd);
                        exit(1);
                    }
                    break;
                }

            }
        }
    }
}



int call_socket(char *hostname, unsigned short portnum) {

    struct sockaddr_in sa;
    struct hostent *hp;
    int s;

    if ((hp= gethostbyname (hostname)) == NULL) {
        print_error ("gethostbyname", errno);
        exit(1);
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr , hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    if ((s = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0) {
        print_error ("socket", errno);
        exit(1);
    }

    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        print_error ("connect", errno);
        exit(1);
    }
    return s;
}




int main(int argc, char* argv[])
{
    if (argc !=4)
    {
        print_client_usage ();
        exit(1);
    }

    for(unsigned int i=0;i<strlen(argv[1]); i++){
        if(i>30 || !isalnum (argv[1][i])){
            print_client_usage ();
            exit(1);
        }
    }

    if(strlen(argv[3])!=4){
        print_client_usage ();
        exit(1);
    }
    int port_num = atoi(argv[3]);
    char * name = argv[1];
    char *ip = argv[2];
    if (gethostbyname (ip) == NULL) {
        print_client_usage ();
        exit(1);
    }
    int fd= call_socket(ip,(unsigned short)port_num);
    if(write_data(fd, name)<0){
        exit_client (fd);
        print_error ("client", errno);
        exit(1);
    }
    cotact_server(fd, name);
}