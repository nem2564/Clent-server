#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <linux/ioctl.h>
#include <linux/rtc.h>
#include <time.h>
#include <pthread.h>


/* ************ Declaring Global variables and Mutex Variables *************************** */

char var_msg[10] = "ABCDEF0123"; /* Message who is going to get copied into value */

int FLAG;												/* Variable used to disable the sender after 10 secs */
int fd,fd1,fd2,fd3,i=0;									/* File descriptor of the four devices*/
int msg =0;												/* This is a the global sequence number */ 
pthread_mutex_t lock_send;								/* Thread mutex for the write function */
pthread_mutex_t lock_recv;								/* Thread mutex for the receive function for the receiver */


typedef struct  										/* The structure of the message to be passed into the queue */
{
    int msg_id;
	int src_id;
	int dest_id;
	char value[10]; 
}packet;

/*********************************** Write function for writing in the queue *******************/
	FILE *p;
void *write_enqueue (void *p)							/* Sending the message to the kernel space for writing into the queue */
{
	pthread_mutex_lock(&lock_send);
  	int res,temp1;
  	temp1 = *((int*) p); 
  while(FLAG) 											/* Wil be disabled after 10 secs */
	{	switch(temp1)
		{
			case (1):						   			/* Creating a message to be send by sender 1 */
				{
					packet pk_send1;
					pk_send1.src_id = 1;
					pk_send1.msg_id = msg;
					pk_send1.dest_id = (rand()%2)+1;	/* Randomizing destination id */
					//pk_send1.acc_time = 0;
					strncpy(pk_send1.value, var_msg, ((rand()%10)+1));						/* Randomizing the length of the character message */
					res =  write(fd, (char *)&pk_send1,sizeof(pk_send1));
					if( res < 0)
					{
						printf("\nBuffer is FULL by Sender 1");
						pthread_mutex_unlock(&lock_send);
						usleep(((rand()%10)+1)*10000);

					}													
					else
					{
						printf("\nData from sender 1 written successfully.\n");
						i++;
						msg++;
						usleep(((rand()%10)+1)*10000);	/* Randomizing sleep */
					}	
					break;
				}


		}	
	}
	printf("\n ********************************************The sender is stopped.*************************************************");
	pthread_exit(0);
}

/***************** Read function to read from the 1st queue and write into the receiver queue *********************************/

void * read_dequeue (void *pk)
{
	packet pkread; 															/* Creating a instance of the message packet */
	int res; 
//	double dummy;
 while(!((FLAG == 0) && (i == 0)))											/* Condition to check whether the sender has stopped sending the messages */
	{
		res  = (int)read(fd, (char *)&pkread, sizeof(pkread));
		usleep(((rand()%100)+1)*10000);
		
		p=fopen("myfile.dat","a");
		fprintf(p, "  %d      ,    %d    ,   %d   .\n", pkread.msg_id, pkread.src_id, pkread.dest_id);


		if(res < 0)
		{
			printf("\n Buffer is EMPTY .");
			usleep(((rand()%10)+1)*10000);	
		}
																			/* Reading the message packet from the queue */
	else{
	//	dummy = (double)(pkread.acc_time/400000);
		switch(pkread.dest_id)												/* Checking the destination id of the packet */
		{
			case(1):														/* Send the message to queue of Receiver 1*/
				{
					printf("\n Data written successfully into the 1st receiver.");
					write(fd1, (char *)&pkread,sizeof(pkread));
					usleep(((rand()%10)+1)*10000);
					break;
				}
			case(2):														/* Send the message to queue of Receiver 2 */
				{
					printf("\n Data written successfully into the 2nd receiver.");
					write(fd2, (char *)&pkread,sizeof(pkread));	
					usleep(((rand()%10)+1)*10000);
					break;
				}
		}	
	    }
	}
	printf("\n****************************************DAEMON OUT************************************");
	pthread_exit(0);
}

/**************************** Read function to read from the receiver queues ************************/

void *read_recv (void *p)
{
  	pthread_mutex_lock(&lock_recv);
  	int ret,temp1;
//	double dummy;
  	temp1 = *((int*) p);
  while(!((FLAG == 0) && (i == 0)))											/* Condition to check whether the sender has stopped sending the messages */
	{
		switch(temp1)
		{
			case (1):														/* Retrieving data from queue of receiver 1 */
				{
					packet pkread1;
					ret = (int)read(fd1, (char *)&pkread1, sizeof(pkread1));
					if(ret < 0)
					{
							printf("\n The buffer 1 of receiver is empty.");
							pthread_mutex_unlock(&lock_recv);
							usleep(((rand()%200)+1)*10000);
					}
					else
					{
						printf("\n Data retrieved from receiver 1.");		/* Indicating the message was read from queue Receiver 1 */
						i--;
						pthread_mutex_unlock(&lock_recv);
						usleep(((rand()%200)+1)*10000);
					}					
				break;
				}

			case (2):														/* Retrieving data from queue of receiver  */
				{
					packet pkread2;
					ret = (int)read(fd2, (char *)&pkread2, sizeof(pkread2));
					if( ret < 0)
						{
							printf("\n The buffer 2 of receiver is is empty.");
							pthread_mutex_unlock(&lock_recv);
							usleep(((rand()%200)+1)*10000);

						}
					else{
						printf("\n Data retrieved from receiver 2.");		/* Indicating the message was read from queue Receiver 1 */
						i--;
						pthread_mutex_unlock(&lock_recv);
						usleep(((rand()%200)+1)*10000);	}	
				break;
				}
		}
	}
	printf("\n *************************************RECEIVER OUT******************************************");
	pthread_exit(0);
}


int main()
{
	time_t begin;	
	
/***************Variables declared to be meant as the sender id i.e. sender 1 or 2 or 3 ************/
	int i=1,j=2,k=3;										

/************************* Opening device files *************************/															

	fd = open("/dev/bus_in_q0", O_RDWR);
	if (fd < 0 ){
		//printf("Can not open device file bus_in_q0.\n");		
		return 0;
	}
	
	fd1 = open("/dev/bus_out_q1", O_RDWR);
	if (fd1 < 0 ){
		//printf("Can not open device file bus_out_q1.\n");		
		return 0;
	}

	fd2 = open("/dev/bus_out_q2", O_RDWR);
	if (fd2 < 0 ){
		//printf("Can not open device file bus_out_q2.\n");		
		return 0;
	}
	FLAG = 1;															/* Setting the value as 1 initially for the sender */ 


    	if((p=fopen("myfile.txt","wb"))==NULL){
      	printf("\nUnable to open file myfile.dat");
      	exit(1);
  		}

  		fprintf(p, "Message_ID, Source_ID, Dest_ID\n");
/***************************** Thread declaration ***********************/
	
	pthread_t thread_send1,thread_send2,thread_daemon,thread_recv1,thread_recv2;
	
/******** Giving a seed value to the srand() function to generate rand() ************/
	srand((unsigned)time(NULL));

/************************* Initializing Mutex **************************/
	pthread_mutex_init(&lock_send, NULL);
	pthread_mutex_init(&lock_recv, NULL);

/******************** Taking the initial time before the execution *****/	
	begin = time(NULL);

/********************* Thread Create***********************************/

	pthread_create(&thread_send1,NULL,write_enqueue,(void *)&i);
//	pthread_create(&thread_send2,NULL,write_enqueue,(void *)&j);
	pthread_create(&thread_daemon,NULL,read_dequeue,NULL);
	pthread_create(&thread_recv1,NULL,read_recv,(void *)&i);
	pthread_create(&thread_recv2,NULL,read_recv,(void *)&j);

	
/******************* Checking whether the 10 sec time limit has passed or not **********/
	while((difftime((time(NULL)), begin) < 10))
	{
			FLAG = 1;
	}
	
/**************** Making FLAG = 0 to make the sender stop sending the data ***********/
	printf("\n **************************The program has ran successfully before flag****************************************\n");

	FLAG = 0;				
	
	pthread_join(thread_send1,NULL);
	printf("\n **************************The program has ran successfully after flag thread_send1****************************************\n");
	pthread_join(thread_daemon,NULL);
	printf("\n **************************The program has ran successfully after flag thread_daemon****************************************\n");
	pthread_join(thread_recv1,NULL);
	printf("\n **************************The program has ran successfully after flag thread_recv1****************************************\n");
	pthread_join(thread_recv2,NULL);
	printf("\n **************************The program has ran successfully after flag thread_recv2****************************************\n");
		close(fd2);
		close(fd1);
		close(fd);
		 fclose(p);
		printf("\n **************************The program has ran successfully****************************************\n");
	exit(0);
}
