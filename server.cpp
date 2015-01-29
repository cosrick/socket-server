#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <sstream>
using namespace std;

int registernum = -1;
string record[20];
string IP[20];
int port[20];
string online[20];
int onlinenum = 0;
string ok = "100 ok \n";
string fail = "210 fail \n";
string bye = "See you \n";
string wrong = "Wrong Input \n";

//the thread function
void *connection_handler(void *);
 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 18753 );
     
    //Bind
    if( ::bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 20);
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0){
            perror("could not create thread");
            return 1;
        } 
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0){
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , buffer[2000];
    bool same = false;
    string ID;
    string buf;
    string reply;
    struct sockaddr_in sa;
	int len = sizeof(sa); 
	char* sentence = (char*)malloc(1000);
    //Receive a message from client
    while( (read_size = recv(sock , buffer , 2000 , 0)) > 0 ){
    
        buffer[read_size] = '\0';
		if(strncmp(buffer, "REGISTER#", 9) == 0){	//register
			buf = buffer;
			ID = buf.substr(9,buf.length());
        	if(registernum >= 0){					//Whether registered
        		for(int i = 0; i <= registernum; i++){
					if(ID == record[i])
						same = true;
				}
        	}
        	if(same){								//registered
        		strcpy(sentence, fail.c_str());
				send(sock, sentence, fail.length(),0);
        	}else{									//Not registered
        		registernum += 1;
        		record[registernum] = ID;
				if(getpeername(sock, (struct sockaddr *)&sa, (socklen_t *)&len) < 0){
					perror("get ID fail!");
					return 0;
				}
        		IP[registernum] = inet_ntoa(sa.sin_addr);
        		port[registernum] = (int)ntohs(sa.sin_port);
        		onlinenum += 1;
        		online[onlinenum-1] = ID;
        		strcpy(sentence, ok.c_str());
				send(sock, sentence, ok.length(),0);	
        	}
        }else if(strncmp(buffer, "List",4) == 0) {		//List
			string s;
 			stringstream ss(s);
			ss << onlinenum;
			reply += ss.str();
        	reply += "\n";
        	for (int i = 0; i < onlinenum; i++){
        		reply += online[i];
        		reply += '#';
        		for (int j = 0; j <= registernum; j++){
        			if(record[j] == online[i]){
        				reply += IP[j];
        				reply += '#';
        				stringstream ss1(s);
        				ss1 << port[j];
        				reply += ss1.str();
        				break;
        			}
        		}
        		reply += "\n";
        	}
        	strcpy(sentence, reply.c_str());
        	send(sock,sentence,reply.length(),0);
        	reply = "";
        }else if(strncmp(buffer, "Exit",4) == 0 ){		//Exit									
			string tempID;
			string tempIP;
			bool hasregistered = false;
        	int tempport = 0;
			if(getpeername(sock, (struct sockaddr *)&sa, (socklen_t *)&len) < 0){
				perror("get ID fail!");
				return 0;
			}
        	tempIP = inet_ntoa(sa.sin_addr);
        	tempport = (int)ntohs(sa.sin_port);
        	for(int i = 0; i <= registernum; i++){
        		if(IP[i] == tempIP && port[i] == tempport){
        			tempID = record[i];
        			hasregistered = true;
        			break;
        		}
        	}
        	if(hasregistered){
        		for(int i = 0; i < onlinenum; i++){
        			if(online[i] == tempID){
        				for(int j = i; j < onlinenum-1; j++)
        					online[j] = online[j+1];
        				break;
        			}
        		}
        		onlinenum -= 1;
        	}
        	strcpy(sentence, bye.c_str());
			send(sock, sentence, bye.length(),0);
			break;
        }else{
        	buf = buffer;
        	if(buf.find('#') != string::npos){
        		string str1;	//ID
        		string str2;	//Port
        		int portnum = 0;
        		bool registered = false;
        		bool offline = true;
        		str1 = buf.substr(0,(buf.find('#')));
        		str2 = buf.substr((buf.find('#')+1),buf.length()-(buf.find('#')+1));
        		//if (isdigit(atoi(str2.c_str())) != 0)
        		portnum = atoi(str2.c_str());
        		/*else{
        			cout << "Wrong with #" << endl;
        			strcpy(sentence, fail.c_str());
        			send(sock, sentence, fail.length(),0);
        			continue;
        		}*/
        		for(int i = 0; i <= registernum; i++){
        			if(record[i] == str1 && port[i] == portnum){
        				registered = true;
        				break;
        			}
        		}
        		for(int i = 0; i < onlinenum; i++){
        			if(online[i] == str1){	
        				offline = false;
        				break;
        			}
        		}
        		if (registered && !offline){
        			string s;
 					stringstream ss(s);
					ss << onlinenum;
					reply += ss.str();
		        	reply += "\n";
        			for (int i = 0; i < onlinenum; i++){
        				reply += online[i];
        				reply += '#';
        				for (int j = 0; j <= registernum; j++){
        					if(record[j] == online[i]){
        						reply += IP[j];
        						reply += '#';
        						stringstream ss1(s);
        						ss1 << port[j];
        						reply += ss1.str();
        						break;
        					}
        				}
        				reply += "\n";
        			}
        			strcpy(sentence, reply.c_str());
        			send(sock,sentence,reply.length(),0);
        			reply = "";	
        		}else if (registered && offline){
        			online[onlinenum] == str1;
        			onlinenum += 1;
        			string s;
 					stringstream ss(s);
					ss << onlinenum;
					reply += ss.str();
		        	reply += "\n";
        			for (int i = 0; i < onlinenum; i++){
        				reply += online[i];
        				reply += '#';
        				for (int j = 0; j <= registernum; j++){
        					if(record[j] == online[i]){
        						reply += IP[j];
        						reply += '#';
        						stringstream ss1(s);
        						ss1 << port[j];
        						reply += ss1.str();
        						break;
        					}
        				}
        				reply += "\n";
        			}
        			strcpy(sentence, reply.c_str());
        			send(sock,sentence,reply.length(),0);
        			reply = "";	
        		}else{
        			strcpy(sentence, fail.c_str());
					send(sock, sentence, fail.length(),0);
        		}		
        	}else{
        		strcpy(sentence, wrong.c_str());
        		send(sock, sentence, wrong.length(),0);
        	}
        }
		
		//clear the message buffer
		memset(sentence,0,strlen(sentence));
		memset(buffer, 0, 2000);
		

    }
	free(sentence);
    if(read_size == 0){
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1){
        perror("recv failed");
    }
         
    return 0;
} 



void List(){
	
}
