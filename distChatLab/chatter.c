/*
 * This system demonstrates the use of the socket() message passing to
 * implement a distributed chat application.  Sockets are different from pipes
 * and shared memory because they can operate BETWEEN computers on a network.
 *
 * The chat app consists of 1 to 100 instances of the program "chatter"
 * which sets up a TCP port and both connects to any existing chatters in the
 * port range 1100-1200 (chosen more or less arbitrarily but causes no conflicts with
 * commonly used ports) and accepts connections from future chatters that join the
 * system.
 *
 * To use simply compile with gcc or the given makefile and then open a new terminal
 * window to run each chatter executable in. Type in any terminal and all other
 * running chatters in the correct port range will receive and output each line.
 * Type "!q" and press enter to quit any individual chatter; any remaining chatters
 * will continue to work no matter which one you quit.
 *
 * Messages are passed through sockets using a local buffer. The format of messages
 * between a client and server is called a PROTOCOL.  As a programmer, YOU have to decide
 * upon a protocol. Currently in this example, a message is simply the string of characters
 * the user types. It starts at buffer[0] and always ends at the first position in the
 * buffer where a '\0' is encountered. You can modify this in any way to add meta data
 * or encode special messages.
 *
 * Author: Sherri Goings
 * Last Modified: 3/6/2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// TODO: remove these notes
// Each node should hold a to-socket and a from-socket
// to-socket: a thing that exists inside cs that the node sends tokens to
//   first node: unset
//   second node: first node
//   all other nodes: unset
//
// from-socket: a thing that exists inside cs that the node receives tokens from
//   first node: unset
//   second node: first node
//   all other nodes: unset
//
// When a node holding the token senses that a new node has joined, it sends
//  the token to that node.
// In the token, say where it's coming from (my node number) and where it should
//  have gone if the new one didn't appear.
//  TODO: see if nodes can know where the messages are coming from
// The new node receives the token and sets the to-socket and from-socket
// If it has anything in the buffer, it sends
// When done with token, pass it along to the next thing.



// messages may be at most 1024 characters long
size_t BUFFER_SIZE = 1024;

// socket address stores important info about specific type of socket connection
struct sockaddr_in address;
int addressSize;

// connected socket array, holds all nConnected sockets for this process, max 100.
int cs[100];
int nConnected = 0;

int to_socket = -1;
int from_socket = -1;

int has_token = 0;


// array to hold threads for receiving messages on a given connection, need 1
// thread per connection so at most 100 total.
pthread_t receivers[100];

// max amount of time in seconds a message may take to travel from one socket to another
int maxDelay = 2;

// an outgoing message has the string to be sent and the number of remaining connections to
// send that message to (initially number of other chatters), so that the message can be
// kept in the buffer of waiting outgoing messages until it has been sent to everyone, then
// removed and the next message can be sent to everyone.
typedef struct sendMessage {
    char message[1024];
    int nToSend;
} sendMessage;

// sendBuffer holds up to 10 messages from this chatter that are waiting to be sent.
// the buffer is implemented as a circular FIFO queue so startIndex is the position of
// the current first message and nHeld is the total number of messages in the buffer
typedef struct toSendBuffer {
    sendMessage messages[10];
    int startIndex;
    int nHeld;
} toSendBuffer;

toSendBuffer sendBuffer;

typedef struct receiveMessage{
    char message[1024];
    // TODO: see if there is anything else I should add here.
} receiveMessage;

// JK addition: receive buffer holds up to 10 messages from another chatter(s)
// circular FIFO queue
typedef struct receivedBuffer {
    receiveMessage messages[10];
    int startIndex;
    int nHeld;
} receivedBuffer;

receivedBuffer rBuffer;

// lock to protect the sendBuffer as multiple threads access it
pthread_mutex_t sendBufLock;
pthread_mutex_t rBufLock;

int most_recent_seq_num;

// JK: current sequence number of message
int seq_num = 1;

int joinNetwork(int port);
int createNetwork(int port);
int acceptConnection(int sock);
void* listenSocket(void*);
int getAndSend();
void* delaySend(void*);
void* acceptIncoming(void*);
int connectCurrent();

int main(int argc, char* argv[])
{
    // user gives port number on which this process should accept incoming connections
    if (argc < 2 || argc > 3) {
        printf("usage: chatter <port number> [max send time (in seconds)]\n");
        return -1;
    }
    if (argc == 3) {
        maxDelay = atoi(argv[2]);
    }

    // initialize global vars where needed
    pthread_mutex_init(&sendBufLock, NULL);
    sendBuffer.startIndex = 0;
    sendBuffer.nHeld = 0;

    pthread_mutex_init(&rBufLock, NULL);
    rBuffer.startIndex = 0;
    rBuffer.nHeld = 0;

    // seed the random number generator with the current time
    srand(time(NULL));

    // first connect to any existing chatters
    if (connectCurrent() == -1) return -1;

    int i;
    // spin off threads to listen to already connected chatters and display their messages
    for (i=0; i<nConnected; i++) {
        pthread_create(&receivers[i], NULL, listenSocket, (void*)(long)i);
    }

   // set up this process's incoming TCP port for future connecting chatters
    int mys;
    mys = createNetwork(atoi(argv[1]));
    if (mys == -1) return -1;

    // spin off thread to listen and connect to future joining chatters
    pthread_t acceptor;
    pthread_create(&acceptor, NULL, acceptIncoming, (void*)(long)mys);

    // use this main thread to get user input and send it to all other chatters
    int quit = 0;
    while (!quit) {
        quit = getAndSend();
    }

    // Cleanup all connections
    for (i=0; i<nConnected; i++) {
        // if disconnected previously, don't try to do so again
        if (cs[i] != -1) {
            shutdown(cs[i], SHUT_RDWR);
        }
    }
    shutdown(mys, SHUT_RDWR);
    return 1;
}

/*
 * sets up a local socket to connect to socket at given port number. Currently
 * connects to given point on local machine, but could connect to distant computer
 * by changing the IP address.
 * argument: port number to attempt to connect to
 * return: -1 on error, 0 on port not found, socket id on successful connection
 */
int joinNetwork(int port) {
    // Create a socket of type stream which gives reliable message passing.
    int s = socket(PF_INET,SOCK_STREAM,0);
    if (s <= 0) {
        printf("client: Socket creation failed.\n");
        return -1;
    }

    // Attempt connection to given port on local machine
    struct sockaddr_in joinAddress;
    int addressSize = sizeof(struct sockaddr_in);
    joinAddress.sin_family=AF_INET;
    joinAddress.sin_port = htons(port);
    inet_pton(AF_INET,"127.0.0.1",&joinAddress.sin_addr);  // <- IP 127.0.0.1

    // If connection is successful, connect will retun 0 and set s appropriately
    if (connect(s,(struct sockaddr*) &joinAddress, addressSize) != 0) {
        return 0;
    }
    return s;
}

/*
 * sets up a local socket to accept incoming connections from other chatters
 * argument: port number to accept connections on
 * return: -1 if fail to create socket or bind socket to port, id of created
 * socket otherwise.
 */
int createNetwork(int port) {
    // Create a socket of type stream which gives reliable message passing.
    int s = socket(PF_INET,SOCK_STREAM,0);
    if (s <= 0) {
        printf("server: Socket creation failed.\n");
        return -1;
    }

    // Create a port to listen to for incoming connections
    addressSize = sizeof(struct sockaddr);
    address.sin_family=AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port  = htons(port);

    // bind the port to the socket
    int status=0;
    status = bind(s,(struct sockaddr*) &address, addressSize);
    if (status != 0) {
        printf("server: Bind failed, unable to create port.\n");
        return -1;
    }

    // listen for incoming connections on this socket, handle a backlog of up
    // to 3 requests
    listen(s,3);

    return s;
}

/*
 * Accepts incoming connection request to given socket. accept is a blocking system
 * call so will sleep until request happens, then be woken up to handle it.
 * argument: socket to check for incoming connections on
 * return: -1 if error, id of new connection if successful.
 */
int acceptConnection(int sock) {
    int newChatter = accept(sock,(struct sockaddr*) &address,(socklen_t *) &addressSize);
    if (newChatter <= 0) {
        printf("server: Accept failed, new chatter can't connect to port.\n");
        return -1;
    }
    return newChatter;
}

/*
 * continually recieves messages on one connected socket. recv is a blocking system call
 * so will sleep until incoming message appears, then be woken up to handle it.
 * argument: index into cs of this connection. conversion to long then int is
 * required to avoid warnings, and I know some of you are very bothered by warnings =)
 */
void* listenSocket(void* args) {
    char* buffer = (char *) malloc(BUFFER_SIZE);
    int index = (int)(long)args;
    int sock = cs[index];
    int size;

    // initial set of the most recently received message's sequence number
    most_recent_seq_num = (int)buffer[0];

    int new_seq_num = 0;

    // recv blocks until message appears.
    // returns size of message actually read, or 0 if realizes connection is lost
    size = recv(sock, buffer, BUFFER_SIZE, 0);
    while (size>0) {
        // handling token receipt
        if (!strcmp(&buffer[2], "token")) {
            int tok_num = (int)buffer[0];
            if (tok_num == -1) {
                // then this node was the first node to enter network
                to_socket = sock;
                from_sock = sock;
                has_token = 1;
            } else {
                // TODO: fill out for general case
            }
        } else {
            new_seq_num = (int)buffer[0];
            printf("new_seq_num: %d\n", new_seq_num);
            // pthread_mutex_lock(&rBufLock);
            if (new_seq_num == most_recent_seq_num + 1) {
                // if the new message is supposed to go right after this one, print
                //  the message and update
                most_recent_seq_num = new_seq_num;
                printf("%d: %s\n", sock, &buffer[1]);
                printf("most_recent_seq_num: %i\n", most_recent_seq_num);

                // now that the message has been saved, search for things in the
                //  rBuffer that can go next
                // since there could be multiple, do an infinite loop and break
                pthread_mutex_lock(&rBufLock);
                while (1) {
                    int i;
                    int index;
                    int cur_seq_num;
                    int found = 0;
                    // for (i = 0; i < rBuffer.nHeld; i++) {
                    for (i = 0; i < 10; i++) {
                        // printf("inside for loop where there are %i entries\n", rBuffer.nHeld);
                        index = (rBuffer.startIndex + i)%10;
                        cur_seq_num = (int)rBuffer.messages[index].message[0];
                        if (cur_seq_num == most_recent_seq_num + 1) {
                            // if there is one found, then print and update
                            most_recent_seq_num = cur_seq_num;
                            printf("The following has been found in buffer\n");
                            printf("%d: %s\n", sock, &rBuffer.messages[index].message[1]);
                            printf("most_recent_seq_num: %i\n", most_recent_seq_num);

                            // then remove that entry from the buffer
                            rBuffer.startIndex = (rBuffer.startIndex + 1) % 10;
                            rBuffer.nHeld --;

                            found = 1;

                            // then start over
                            break;
                        } else {
                            // printf("cur_seq_num: %i\n", cur_seq_num);
                            // printf("most_recent_seq_num: %i\n", most_recent_seq_num);
                        }
                    }
                    if (!found) {
                        // there are no entries directly following recent
                        // printf("entries remaining in rBuffer: %i\n", rBuffer.nHeld);
                        break;
                    }
                    // if there was something found, maybe there is another.
                    // look again.
                }
                pthread_mutex_unlock(&rBufLock);
            } else {
                // otherwise put message in the received buffer


                pthread_mutex_lock(&rBufLock);
                if (rBuffer.nHeld == 10) {
                    pthread_mutex_unlock(&rBufLock);
                    printf("\n\nERROR: receive buffer full, undefined behavior for this case, exiting instead.\n\n");
                    exit(1);
                }

                int index = (rBuffer.startIndex+rBuffer.nHeld)%10;
                strcpy(rBuffer.messages[index].message, buffer);
                rBuffer.nHeld++;
                pthread_mutex_unlock(&rBufLock);
                printf("just put %d in the rBuffer\n", new_seq_num);

            }
        }



        size = recv(sock, buffer, BUFFER_SIZE, 0);
     }

    // socket was closed by other side so close this end as well
    shutdown(cs[index], SHUT_RDWR);
    cs[index] = -1;
	return 0;
 }

/*
 * continually get input from this chat user and send to all connected chatters.
 * also does some formatting to make terminal look more like typical chat window to user.
 * return 1 if user enters command to quit, 0 otherwise.
 */
int getAndSend() {
    char* buffer = (char *) malloc(BUFFER_SIZE);
    // if (nConnected == 2) {
    //     printf("here!\n");
    // }
    if ((nConnected == 1) && (from_socket == -1) && (to_socket == -1)) {
        printf("I should be the second node to connect\n");
        // if this is the second node to connect, create the token and the ring
        // printf("%i\n",cs[nConnected - 1]);
        // printf("%i\n",cs[nConnected ]);
        // printf("%i\n",cs[nConnected + 1]);
        from_socket = cs[nConnected - 1];
        to_socket = cs[nConnected - 1];
        has_token = 1;
        char num[1];
        num[0] = (char)-1;
        strcpy(&buffer[0], num);
        strcpy(&buffer[1], "token");
        // buffer =
    } else {
        // Get keyboard input from user, strip newline, quit if !q
        ssize_t nChars = getline(&buffer, &BUFFER_SIZE, stdin);
        buffer[nChars-1] = '\0';
        if (strcmp(buffer,"!q")==0) return 1;

        // clear line user just entered (only want to display lines when we choose in case need to reorder)
        printf("\33[1A\r");
        printf("\33[2K\r");
    }










    // check that sendBuffer is not full
    pthread_mutex_lock(&sendBufLock);
    if (sendBuffer.nHeld == 10) {
        pthread_mutex_unlock(&sendBufLock);
        printf("\n\nERROR: send buffer full, undefined behavior for this case, exiting instead.\n\n");
        exit(1);
    }
    // JK: add a number (sequence number) to the beginning of the message
    char tmp[1024];
    tmp[0] = (char)seq_num;

    // copy the buffer over
    strcpy(&tmp[1], buffer);

    // copy the temp stuff back
    strcpy(buffer, tmp);


    // printf("%d\n", tmp[0]);
    // printf("%s\n", tmp);
    // printf("this message's sequence number: %i\n", seq_num);
    // printf("size of the string: %lu\n", sizeof(str));

    // copy message to send buffer at appropriate index and initalize num remaining to send to 0
    int index = (sendBuffer.startIndex+sendBuffer.nHeld)%10;
    sendBuffer.messages[index].nToSend = 0;
    strcpy(sendBuffer.messages[index].message, buffer);
    sendBuffer.nHeld++;

    pthread_mutex_unlock(&sendBufLock);
    // Send message to all connections except those that have already been closed
    int i;
    for (i=0; i<nConnected; i++) {
        if (cs[i] != -1) {

            // increment num remaining to send of this message
            pthread_mutex_lock(&sendBufLock);
            sendBuffer.messages[index].nToSend++;
            pthread_mutex_unlock(&sendBufLock);

            // create thread to do the actual sending so can add delays, needs to know
            // index of message in sendBuffer and socket to send to. Totally cheating and using the fact
            // that a void* is 64 bits and each of these args is 32 bits so simply putting the index in
            // the 1st 32 bits of args and the socket in the 2nd 32 bits. Note that something seemingly
            // more logical like creating an array of the 2 integers and passing the address won't work
            // because the array will only exist until this function ends, before the threads actually
            // need to use it.
            long indexL = (long)index;
            void* args = (void*)((indexL << 32) + cs[i]);
            pthread_t send_thread;
            pthread_create(&send_thread, NULL, delaySend, args);
         }
    }

    // print line in my own window, currently paying no attention to order
    printf("me: %s\n", &buffer[1]);

    seq_num++;

    return 0;
}

/*
 * thread to sleep for random amount of time between 0 and given max # seconds, then send message.
 * update sendBuffer as appropriate
 * argument: first 32 bits of void* is index of message in sendBuffer, last 32 bits give
 * socket to send message to.
 */
void* delaySend(void* args) {
    // reversing process above to get index and socket out of args
    long argsL = (long)args;
    int index = (int)(argsL >> 32);
    int socket = (int)((argsL << 32) >> 32);

    // delay random amount up to max allowed
    usleep(rand()%(maxDelay*1000000));

    pthread_mutex_lock(&sendBufLock);
    send(socket, sendBuffer.messages[index].message, BUFFER_SIZE, 0);

    // once send has completed, decrement num remaining to send of this message, if this was the last
    // "remove" this message from send buffer (actually just move startIndex and decrement nHeld)
    sendBuffer.messages[index].nToSend--;
    if (sendBuffer.messages[index].nToSend == 0) {
        sendBuffer.startIndex = (sendBuffer.startIndex+1)%10;
        sendBuffer.nHeld--;
    }
    pthread_mutex_unlock(&sendBufLock);
	return 0;
}

/*
 * wait for incoming connections. When connection is made, save to next slot in connected
 * socket array (cs) and create a new receiver thread to listen to new chatter. Note that main
 * thread will automatically start sending to this new chatter as well because of updated
 * nConnected. acceptConnection function makes a blocking call so we don't need to worry about
 * busy waiting.
 * argument is socket id to look for incoming requests on. conversion to long then int is
 * required to avoid warnings, and I know some of you are very bothered by warnings =)
 */
void* acceptIncoming(void* args) {
    int sock = (int)(long)args;
    while (1==1) {
        int newC = acceptConnection(sock);
        if (newC == -1) exit(1);
        cs[nConnected] = newC;
        pthread_create(&receivers[nConnected], NULL, listenSocket, (void*)(long)nConnected);
        nConnected++;
    }
}

/*
 * scan all ports from 1100 to 1200 for already existing chatters by attempting
 * to connect to each. If successfully connect to a port, save connection in next slot
 * in connected socket array (cs), otherwise do nothing.
 * return -1 if encounter failure in creating socket for connection on this end
 */
int connectCurrent() {
    int i;
    for (i=1100; i<1200; i++) {
        int s = joinNetwork(i);
        if (s == -1) return -1;
        if (s > 0) {
            printf("connected to socket at port %d\n", i);
            cs[nConnected++] = s;
        }
    }
    return 0;
}
