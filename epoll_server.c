#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <signal.h>


int main(int argc, char **argv)
{
	//Port Variable
	int port = 0;

	//No Command Line Argument
  	if(argc == 1)
	{
		exit(1);
	}
	else
	{
		//Check if Argument is All Numbers
		for(int i = 0; argv[1][i] != '\0'; i++)
		{
			//Argument is Not Integer
			if(!isdigit(argv[1][i]))
			{
				exit(1);
			}
		}

		//Convert Char to Integer
		port = strtol(argv[1], NULL, 10);

		//Port Number Out of Range
		if(port > 65535 || port < 1024)
		{
			exit(1);
		}
	}

	//Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	//Server Socket Variable
  	int sockfd = 0;

	//Create Socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//Error in Socket Creation
	if(sockfd == -1)
	{
		exit(1);
	}

	//Server Address Variable, sockaddr_internet
	struct sockaddr_in serverAddress;

	//Clear Server Address Memory
	memset(&serverAddress, '\0', sizeof(serverAddress));
	//Set to IPv4 Address
	serverAddress.sin_family = AF_INET;
	//Set Server Address Port Number
	serverAddress.sin_port = htons(port);
	//Set Accepted Addresses
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	//Bind Error
	if(bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
	{
		//Close Server Socket
		close(sockfd);

		exit(1);
	}
	
	//Set up Max Number of Partial Connections Being Established
	if(listen(sockfd, 50) == -1)
	{
		//Close Server Socket
		close(sockfd);

		exit(1);
	}

	//Message Inputted By Client
	char message[76];

	//Length of Input Message
	int message_len = 0;

	//Server Post
	char *post = (char *)malloc(75*sizeof(char));

	//Post Memory Allocation Failed
	if(post == NULL)
	{
		exit(1);
	}

	//Clear Post Memory
	memset(post, 0, 75*sizeof(char));

	//Initialize Post
	strcpy(post, "\n");

	//Create Epoll
	int epollfd = epoll_create1(0);
 
	//Epoll Create Error
  	if(epollfd == -1)
  	{
  		//Close Server Socket
		close(sockfd);

    	exit(1);
  	}

  	//Create Epoll Event
  	struct epoll_event event;
  	struct epoll_event *events = calloc(50, sizeof(event));

  	//Events Memory Allocation Failed
	if(events == NULL)
	{
		exit(1);
	}

  	//Assign Event Types
  	event.events = EPOLLIN;
  	event.data.fd = sockfd;

  	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1)
  	{
    	//Close Epoll
    	close(epollfd);

    	//Close Server Socket
    	close(sockfd);

		exit(1);
	}

    //New Socket to Communicate With Client
	int clientfd;

	//Client Address
	struct sockaddr clientAddress;

	//Client Address Length
	socklen_t addrlen = sizeof(clientAddress);

    //Number of Events
    int events_num = 0;

    //Server Infinitely Runs
	while(1)
	{
		//Get Number of Waiting Clients
		events_num = epoll_wait(epollfd, events, 50, -1);

		for(int i = 0; i < events_num; i++)
		{
			if(events[i].data.fd == sockfd)
			{
				clientfd = accept(sockfd, &clientAddress, &addrlen);

				//Error Creating Client Socket
				if (clientfd != -1)
				{
					//Fill in Event Struct
					event.events = EPOLLIN;
                    event.data.fd = clientfd;

					epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &event);
				}
			}
			else
			{
				//Clear Message String, Unnecessary When Testing
				memset(message , 0, strlen(message));

				//Read Input Message From Client
				message_len = read(events[i].data.fd, &message, 76);

				//Client Read Successful
				if(message_len > 0 && message_len <= 76)
				{
					//Check if Client is Asking For Current Post
					if(strcmp(message, "?\n") == 0)
					{
						//Send Current Post to Client
						if(write(events[i].data.fd, post, strlen(post)) <= 0)
						{
							//Close Client
							close(events[i].data.fd);
						}
					}
					//Check if Message Begins With '!' And Ends in '\n'
					else if(message[0] == '!' && message[strlen(message) - 1] == '\n')
					{
						//Clear Old Post
						memset(post, 0, 75*sizeof(char));

						//Exract Legal Part From Multiple Newlines and Set New Post
						memmove(post, message + 1, (int)(strchr(message, '\n') - message));
					}
				}
				else if(message_len <= 0)
				{
					//Close Client
					close(events[i].data.fd);

					//Remove Epoll Event
					epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
			}
		}
	}

    //Close Epoll
    close(epollfd);

    //Close Server Socket
	close(sockfd);

	//Free Events Pointer
	free(events);

	//Free Post Pointer
	free(post);

  	return 0;
}



