int setupServerSocket(int portno, int connectionType); // Like new ServerSocket in Java
int callServer(char* host, int portno); // Like new Socket in Java
int serverSocketAccept(int serverSocket); // Like ss.accept() in Java
void writeInt(int x, int socket); // Write an int over the given socket
int readInt(int socket); // Read an int from the given socket
void writeBoolArray(bool* prime, int socket, int arrayLength);
bool* readBoolArray(int socket, int arrayLength);


