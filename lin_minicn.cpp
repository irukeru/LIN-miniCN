/*********************************************************************************************
 **                                                                                         **
 ** @author         : Ilker MORAL	                                                    **
 ** @version        : 1                                                                     **
 ** @email          : ilkermoral@gmail.com                                                  **
 ** @since          : Jul 25 01:34:12 2014                                                  **
 ** @compiled with  : G++ GCC4.7.2 (Debian 4.7.2-5)                                         **
**********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    	//strlen
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> 	//inet_addr
#include <iostream>
#include <syslog.h>	//log

/*
 * add /etc/syslog.conf file
 * local1.info /var/log/lin_miniCN.log
 * pkill -HUP syslogd
 */
 
using namespace std;

int socket_desc;
struct sockaddr_in server;
char *message = "OK\r\n";
char server_reply[2000];
int flag_ofile = 0;
string file_name;

void write_to_file(std::string, char*);
void configure(string, int);
int connectSocket();
string charToString(char *);
void signalHandler( int );

string addrStr = "127.0.0.1";
int port = 2343;

/**
 *
 */
int main(int argc , char *argv[]) {

	configure(addrStr, port);

	connectSocket();
          
	while(1) {
		int con_status =  recv(socket_desc, server_reply , 2000 , 0);

		if( con_status > 0) {
      			cout << "received data : " << server_reply;

		    	if (flag_ofile == 1 && server_reply[0] != '$') {
				write_to_file(file_name, server_reply);
		    	} else if (server_reply[0] == '$' && server_reply[1] == 'O' && server_reply[2] == 'F' && flag_ofile == 0) {

				if ((int)server_reply[3] == 10) {
					cerr << "error : no file name received" << endl;
					syslog(LOG_ERR, "no file name received to open a file");
					continue;
				}


				try {
					file_name = charToString(server_reply);
		
				    	if( send(socket_desc , message , strlen(message) , 0) < 0) {
						printf("Connection failed \n");
						continue;
 		    		    	}

					cout << "send data : OK" << endl; 

				    	flag_ofile = 1;

					syslog(LOG_INFO, "%s file name has received.", file_name.c_str());

				} catch (int e) {
					cerr << "error : " << e << endl;
					syslog(LOG_ERR, "exception : %d", e);
				}

  		    	} else if (server_reply[0] == '$' && server_reply[1] == 'C' && server_reply[2] == 'F' && flag_ofile == 1) {

			    	if( send(socket_desc , message , strlen(message) , 0) < 0) {
					printf("error : Connection failed \n");
					return 1;
			    	}

		    		flag_ofile = 0;

				cout << "send data : OK" << endl; 

				syslog(LOG_INFO, "%s has closed.", file_name.c_str());
		    	}

			memset(server_reply, 0, sizeof server_reply);;
	    
		} else {
			syslog(LOG_ERR, "connection closed");
			connectSocket();
		}

		usleep(100000);
	}   
     
	closelog();

    	return 0;
}

/*
 * @param file_name
 * @param data
 * writes data to file
 */
void write_to_file(std::string file_name, char* data) {

	FILE* file;

        char chr_file_name[file_name.length() + 100];
        strcpy(chr_file_name, file_name.c_str());

        file = fopen(chr_file_name, "a");

	fprintf(file, "%s", data);

	syslog(LOG_INFO, "received data : %s", data);

	fclose(file);
}

void configure(string addr, int port) {

	signal(SIGBUS, signalHandler);
	signal(SIGINT, signalHandler);

	openlog("lin_miniCN.log", LOG_CONS, LOG_LOCAL1);

	cout << "LIN-miniCN has started." << endl;

	syslog(LOG_INFO, "LIN-miniCN has started.");

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
        	printf("Could not create socket");
    	}

    	server.sin_addr.s_addr = inet_addr(addr.c_str());
    	server.sin_family = AF_INET;
    	server.sin_port = htons( port );

	syslog(LOG_INFO, "Configured as %s:%d", addrStr.c_str(), port);
}

int connectSocket() {

	syslog(LOG_INFO, "listening port %d", port);

	while (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0) {
        	puts("awaiting connection");
		usleep(100000);
		socket_desc = socket(AF_INET , SOCK_STREAM , 0);
		if (socket_desc == -1) {
			printf("Could not create socket");
		}
    	}

	syslog(LOG_INFO, "Connection established");

	cout << "Connected" << endl;

	return 1;
}

string charToString(char * filename) {
	string result = "";
	
	// checking CRLF

	for(int i = 4; (int)filename[i] != 10 ; i++) {
		result += filename[i];	
	}

	return result;
}


void signalHandler( int signum ) {
    	cout << "Interrupt signal (" << signum << ") received.\n";

	syslog(LOG_ERR, "service stopped immediately. interrupt signal %d received.", signum);

	closelog();

   	exit(signum);
}
