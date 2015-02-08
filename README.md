# Clent-server
Enqueue and Dequque operation for client serve model
CSE530 Project 3 – Build your own kernel driver

Device Driver for simple router and message queue

Introduction:
I am thinking about writing device driver which holds incoming message to data structure and then through message bus which manages the connections and sends the messages to desired locations. Message bus also keeps track of messages sent to receivers and generates table which contains record of message arrival. I am also thinking about putting some visual indications like LEDs. This project will includes thread generation, synchronization, locking, device driver, complex data structure, message passing, table generation and GPIO operation and timing.

Diagram:
 
Description:
•	This project includes two files mainly: 1) Driver file and 2) Application file.
•	1) Driver file comprises write module and read module with semaphore for synchronization and provision for race condition. Gpio feature for LED operation which indicates write operation every time when thread acquire lock. Also Three device with name bus_in_queue0, bus_out_queue1, bus_out_queue2 which replicates input and output of the router. 
•	2) Application file has 4 threads, one writes to queue, one router operates, two read from output. Thread operation also restricted by mute locking to avoid conflicts to obtain critical section.
 

•	Note: when you run application, all threads are starting simultaneously. Initially output of both receivers shows empty results and receiver threads try to read data from random locations of respective receiver. When application finishes execution after 10 seconds, this output will be shown on screen.
•	I am also attaching video showing LED glowing while write operation is taking place.
