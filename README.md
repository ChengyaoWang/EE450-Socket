# EE450-Socket
This is the implementation for EE450's Project @ USC

## Clarifications
- All assertion checks for the validity of send/receive in both TCP & UDP is skipped.
  - Assertions for network establishment is will kept
- All printouts are as neat as possible
  - Contents may lead to ambiguity
  - Their major job is to show the success of operations
- Please boot up in the order
  - Server A & B -> AWS -> Moniter -> Client
  - One-click-for-all may be implemented later using *fork()*
- Unlike Project's original requirements, "client" here accepts continous commandline input
- Supported Commands (***Client***):
  - ***"test"***. Starting from client, ping all components of the network
  - ***"exit"***. Starting from client, require all components of the network to terminate.
  - ***"write \<BW> \<LENGTH> \<VELOCITY> \<NOISEPOWER>"***. Append 1 record to the database
  - ***"compute \<LINK_ID> \<SIZE> \<SIGNALPOWER>"***. Compute & Printout the results as required.
  - ***Malformed Commands are not tested ! Please only input supported commands***.
  - ***All Variables are in INT32 format, except \<LENGTH> in DOUBLE***.
- Supported Commands (***Makefile***):
  - ***"make"***. Compute & Link all Source Files
  - ***"make clean"***. Clean all executable & *.o files
