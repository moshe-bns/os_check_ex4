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

int write_data(int s, void *buff) {
    char* buf = (char *)buff;
    char send_buf[MESGSIZE];
    bzero (send_buf, MESGSIZE);
    strcpy (send_buf, buf);
    std::string x(buf);
    std::string y(send_buf);
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

void server_down(std::map<std::string,int> *client_list){
    for (auto it=client_list->begin(); it!=client_list->end(); ++it){
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,SERVFAIL);
        if(write_data(it->second, _mes)<0){
            server_down (client_list);
            exit(1);
        }
        close (it->second);
    }
}


int establish (unsigned short portnum)
{
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    char my_name[MAXHOSTNAME+1];

    //hostnet initialization
    gethostname(my_name, MAXHOSTNAME);
    hp = gethostbyname(my_name);
    if (hp == NULL)
    {
        print_error ( "gethostbyname" , errno);
        exit(1);
    }

    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    /* this is our host address */
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    /* this is our port number */
    sa.sin_port= htons(portnum);
    /* create socket */
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0){
        print_error ( "socket" , errno);
        exit(1);
    }
    sa.sin_addr.s_addr=INADDR_ANY;
    if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
        close(s);
        print_error ( "bind" , errno);
        exit(1);
    }
    listen(s, 10); /* max # of queued connects */
    return(s);
}

int get_connection(int my_fd) {
    int t; /* socket of connection */
    if ((t = accept(my_fd,NULL,NULL)) < 0){
        print_error ("accept", errno);
        exit(1);
    }
    return t;
}

void connectNewClient (int s, fd_set* all_fd, std::map<std::string,int> *client_list, std::map<std::string,std::vector<std::string>> *group_list)
{
    int fd_client = get_connection(s);
    char buf[MESGSIZE];
    memset(buf, 0, MESGSIZE);
    if(read_data(fd_client, buf)<0){
        server_down (client_list);
        exit(1);
    }
    std::string client_name = strtok(buf, "\r\n");
    if (client_list->find(client_name) != client_list->end() || group_list->find(client_name) != group_list->end() )
    {
        //-------------should also print in server "printf("Client name is already in use.\n")"
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,CONEXIST);
        if(write_data(fd_client, _mes)<0){
            server_down (client_list);
            exit(1);
        }
        close(fd_client);
    }
    else
    {
        (*client_list)[ client_name ] = fd_client;
        FD_SET( fd_client , all_fd );
        char mes[MESGSIZE];
        bzero (mes,MESGSIZE);
        strcpy (mes,CONSUC);
        if(write_data(fd_client, mes)<0){
            server_down (client_list);
            exit(1);
        }
        print_connection_server ( client_name );
    }
}

void serverStdInput(std::map<std::string,int> *client_list, std::map<std::string,std::vector<std::string>> *group_list){
    char buf[MESGSIZE];
    bzero (buf,MESGSIZE);
//    read_data(STDIN_FILENO, buf);
    std::string mes_std_in;
    std::getline (std::cin,mes_std_in);
    if(mes_std_in.empty ()){
        return;
    }
    strcpy (buf,mes_std_in.c_str ());
    std::string command = strtok(buf, "\r\n");
    if(command.compare("EXIT")==0)
    {
        print_exit ();
        server_down (client_list);
        group_list->clear ();
        client_list->clear ();

        exit(0);
    }
}
void exit_client(int fd_client,std::string name, std::map<std::string,int> *client_list, std::map<std::string,std::vector<std::string>> *group_list, fd_set* all_fd, bool flag=true)
{
    std::vector<std::string> remove_group;
    close(fd_client);
    FD_CLR (fd_client, all_fd);
    client_list->erase (name);
    for(auto it=group_list->begin ();it!=group_list->end ();it++){
        auto it_vec = std::find(it->second.begin (),it->second.end (), name);
        if(it_vec!=it->second.end ()){
            it->second.erase (it_vec);
        }
        if(it->second.size ()==1){
            remove_group.push_back(it->first);
//            group_list->erase (group_list->find (it->first));
        }
    }
    for(auto it=remove_group.begin ();it!=remove_group.end ();it++){
        group_list->erase (group_list->find (*it));
    }
    remove_group.clear();
    if(flag)
    {
        print_exit ( true , name );
    }
}

void who_func(int fd_client,std::string name, std::map<std::string,int> *client_list){
    std::string clients = WHOMES;
    std::vector<std::string> client_list_vec;
    for(auto it = client_list->begin ();it!=client_list->end ();it++){
        client_list_vec.push_back (it->first);
    }
    std::sort(client_list_vec.begin (), client_list_vec.end());
    for (auto it = client_list_vec.begin (); it!=client_list_vec.end ();it++){
        clients+=*it;
        clients+=',';
    }
    clients = clients.substr (0, clients.size ()-1);
    char _mes[MESGSIZE];
    bzero (_mes, MESGSIZE);
    strcpy (_mes,clients.c_str ());
    if(write_data(fd_client, _mes)<0){
        server_down (client_list);
        exit(1);
    }
    print_who_server (name);
}

void send_func_group(int fd_client,std::string client_name, std::map<std::string,int> *client_list,
               std::map<std::string,std::vector<std::string>> *group_list, std::string& dest_name, std::string& message)
{
    std::string mes_to_sender = SENDMES;
    std::string delim = ":";
    std::string mes_to_reciever = GETMES;
    for(auto it=(*group_list)[dest_name].begin (); it!=(*group_list)[dest_name].end ();it++){
        if(client_name.compare (*it)==0){
            continue;
        }
        int fd= (*client_list)[*it];
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_reciever+client_name+delim+message).c_str ());
        if(write_data(fd, _mes)<0){
            server_down (client_list);
            exit(1);
        }
    }
        print_send (true, true, client_name,dest_name,message);
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_sender).c_str ());
        if(write_data(fd_client, _mes)<0){
            server_down (client_list);
            exit(1);
        }
}
void send_func(int fd_client,std::string client_name, std::map<std::string,int> *client_list,
               std::map<std::string,std::vector<std::string>> *group_list, std::string& dest_name, std::string& message){
    auto it = client_list->find (dest_name);
    if(it==client_list->end () || client_name.compare (dest_name) ==0){
        //check group
        if(group_list->find (dest_name)!=group_list->end ())
        {
            if( std::find ((*group_list)[dest_name].begin (), (*group_list)[dest_name].end (), client_name)!=(*group_list)[dest_name].end ()){
                send_func_group(fd_client,client_name, client_list, group_list, dest_name, message);
                return;
            }
        }
        std::string mes_to_sender = SENDMESFAIL;
        print_send (true, false, client_name,dest_name,message);
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_sender).c_str ());
        if(write_data(fd_client, _mes)<0){
            server_down (client_list);
            exit(1);
        }
    }
    else{
        std::string mes_to_sender = SENDMES;
        std::string delim = ":";
        std::string mes_to_reciever = GETMES;
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_reciever+client_name+delim+message).c_str ());
        if(write_data(it->second, _mes)<0){
            server_down (client_list);
            exit(1);
        }
        print_send (true, true, client_name,dest_name,message);
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_sender).c_str ());
        if(write_data(fd_client, _mes)<0){
            server_down (client_list);
            exit(1);
        }

    }
}


void create_group_func(int fd_client,std::string client_name, std::map<std::string,int> *client_list,
               std::map<std::string,std::vector<std::string>> *group_list, std::string& group_name ,
               std::vector<std::string>* group_member){
    int is_group = 0;
    int is_client = 0;
    int is_name_not_exists = 0;
    auto it_client = client_list->find (group_name);
    auto it_group = group_list->find (group_name);
    if(it_client!=client_list->end ()){
        is_client=1;
    }
    if(it_group != group_list->end ()){
        is_group=1;
    }
    for(auto it= group_member->begin ();it!=group_member->end();it++){
        if(client_list->find (*it) ==client_list->end()){
           is_name_not_exists=1;
            break;
        }
    }
    std::string &some_name = group_member->back ();
    if( is_name_not_exists || is_group || is_client  ||group_member->empty () ||(group_member->size ()==1 && some_name.compare (client_name)==0)){
        std::string mes_to_sender = GROUPFAIL;
        print_create_group ( true, false,client_name, group_name);
        char _mes[MESGSIZE];
        bzero (_mes, MESGSIZE);
        strcpy (_mes,(mes_to_sender+group_name).c_str ());
        if(write_data(fd_client, _mes)<0){
            server_down (client_list);
            exit(1);
        }
        return;
    }
    std::vector<std::string> group_member_unique;
    group_member->push_back (client_name);
    for(auto it=group_member->begin ();it!=group_member->end ();it++){
        if(std::find (group_member_unique.begin (),group_member_unique.end (),*it)==group_member_unique.end ()){
            group_member_unique.push_back (*it);
        }
    }
    (*group_list)[group_name] = group_member_unique;
    std::string mes_to_sender = GROUPSUC;
    char _mes[MESGSIZE];
    bzero (_mes, MESGSIZE);
    strcpy (_mes,(mes_to_sender+group_name).c_str ());
    if(write_data(fd_client, _mes)<0){
        server_down (client_list);
        exit(1);
    }
    print_create_group ( true, true,client_name, group_name);


}
void wait_connection(int serverSockfd, fd_set* all_fd,  std::map<std::string,int> *client_list, std::map<std::string,std::vector<std::string>> *group_list)
{
    fd_set readfds;
    while (true)
    {
        readfds = *all_fd;
        if ( select (MAX_CLIENTS + 1 , &readfds , NULL , NULL , NULL ) < 0 )
        {
            server_down (client_list);
            print_error ("select", errno);
            exit(1);
        }
        if (FD_ISSET(serverSockfd , &readfds))
        {
            //will also add the client to the clientsfds
            connectNewClient ( serverSockfd , all_fd , client_list, group_list);
        }
        if ( FD_ISSET( STDIN_FILENO , &readfds ))
        {
             serverStdInput (client_list,group_list);
        }

        std::map<std::string, int> remove_clients;
        for (auto it=client_list->begin(); it!=client_list->end(); ++it){
            if (it->second==serverSockfd || it->second == STDIN_FILENO){
                continue;
            }
            if(FD_ISSET (it->second, &readfds)){
                char buf[MESGSIZE];
                if(read_data(it->second, buf)<0){
                    server_down (client_list);
                    exit(1);
                }
                if ( strcmp ( buf , CLIENTFAIL ) == 0 )
                {
                    remove_clients[it->first]=it->second;
//                    exit_client (it->second,it->first, client_list,group_list, all_fd, false);
                    continue;
                }
                std::string command = strtok(buf, "\r\n");
                command_type commandT;
                std::string name;
                std::string message;
                std::vector<std::string> clients;
                parse_command (command,commandT,name,message,clients);
                switch (commandT){
                    case EXIT: remove_clients[it->first]=it->second; break;
                    case WHO: who_func(it->second,it->first, client_list);break;
                    case SEND: send_func(it->second,it->first, client_list,group_list, name, message);break;
                    case CREATE_GROUP: create_group_func(it->second,it->first, client_list,group_list, name, &clients);break;
                    case INVALID:break;
                }
            }
        }
        for (auto it=remove_clients.begin(); it!=remove_clients.end(); ++it){
            exit_client (it->second,it->first, client_list,group_list, all_fd);
        }
        remove_clients.clear();
    }
}


int main(int argc, char* argv[]){
    if (argc<1){
        print_server_usage ();
        exit(1);
    }
    unsigned short portnum = (unsigned short)atoi(argv[1]);
    int my_fd = establish (portnum);
    std::map<std::string, int> client_list; //string = name of client, int = fd
    std::map<std::string,std::vector<std::string>> group_list;
    fd_set all_fd;
    FD_ZERO (&all_fd);
    FD_SET(STDIN_FILENO, &all_fd);
    FD_SET(my_fd, &all_fd);
    wait_connection (my_fd, &all_fd, &client_list, &group_list);

}