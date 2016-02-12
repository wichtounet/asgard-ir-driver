//=======================================================================
// Copyright (c) 2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

extern "C" {
#include <lirc/lirc_client.h>
}

namespace {

const std::size_t UNIX_PATH_MAX = 108;
const std::size_t buffer_size = 4096;

// Configuration (this should be in a configuration file)
const char* server_socket_path = "/tmp/asgard_socket";
const char* client_socket_path = "/tmp/asgard_ir_socket";

//Buffers
char write_buffer[buffer_size + 1];
char receive_buffer[buffer_size + 1];

// The socket file descriptor
int socket_fd;

// The socket addresses
struct sockaddr_un client_address;
struct sockaddr_un server_address;

// The remote IDs
int source_id = -1;
int button_actuator_id = -1;

void stop(){
    std::cout << "asgard:ir: stop the driver" << std::endl;

    //Closes LIRC
    lirc_deinit();

    // Unregister the button actuator, if necessary
    if(temperature_sensor_id >= 0){
        auto nbytes = snprintf(write_buffer, buffer_size, "UNREG_ACTUATOR %d %d", source_id, button_actuator_id);
        sendto(socket_fd, write_buffer, nbytes, 0, (struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
    }

    // Unregister the source, if necessary
    if(source_id >= 0){
        auto nbytes = snprintf(write_buffer, buffer_size, "UNREG_SOURCE %d", source_id);
        sendto(socket_fd, write_buffer, nbytes, 0, (struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
    }

    // Unlink the client socket
    unlink(client_socket_path);

    // Close the socket
    close(socket_fd);
}

void terminate(int){
    stop();

    std::exit(0);
}

void ir_received(char* raw_code){
    std::string full_code(raw_code);

    auto code_end = full_code.find(' ');
    std::string code(full_code.begin(), full_code.begin() + code_end);

    auto repeat_end = full_code.find(' ', code_end + 1);
    std::string repeat(full_code.begin() + code_end + 1, full_code.begin() + repeat_end);

    auto key_end = full_code.find(' ', repeat_end + 1);
    std::string key(full_code.begin() + repeat_end + 1, full_code.begin() + key_end);

    std::cout << "asgard:ir: Received: " << code << ":" << repeat << ":" << key << std::endl;

    //Send the event to the server
    auto nbytes = snprintf(write_buffer, buffer_size, "EVENT %d %d %s", source_id, button_actuator_id, key.c_str());
    sendto(socket_fd, write_buffer, nbytes, 0, (struct sockaddr *) &server_address, sizeof(struct sockaddr_un));
}

} //End of anonymous namespace

int main(){
    //Initiate LIRC. Exit on failure
    char lirc_name[] = "lirc";
    if(lirc_init(lirc_name, 1) == -1){
        std::cout << "asgard:ir: Failed to init LIRC" << std::endl;
        return 1;
    }

    // Open the socket
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(socket_fd < 0){
        std::cerr << "asgard:ir: socket() failed" << std::endl;
        return 1;
    }

    // Init the client address
    memset(&client_address, 0, sizeof(struct sockaddr_un));
    client_address.sun_family = AF_UNIX;
    snprintf(client_address.sun_path, UNIX_PATH_MAX, client_socket_path);

    // Unlink the client socket
    unlink(client_socket_path);

    // Bind to client socket
    if(bind(socket_fd, (const struct sockaddr *) &client_address, sizeof(struct sockaddr_un)) < 0){
        std::cerr << "asgard:ir: bind() failed" << std::endl;
        return 1;
    }

    //Register signals for "proper" shutdown
    signal(SIGTERM, terminate);
    signal(SIGINT, terminate);

    // Init the server address
    memset(&server_address, 0, sizeof(struct sockaddr_un));
    server_address.sun_family = AF_UNIX;
    snprintf(server_address.sun_path, UNIX_PATH_MAX, server_socket_path);

    socklen_t address_length = sizeof(struct sockaddr_un);

    // Register the source
    auto nbytes = snprintf(write_buffer, buffer_size, "REG_SOURCE ir");
    sendto(socket_fd, write_buffer, nbytes, 0, (struct sockaddr *) &server_address, sizeof(struct sockaddr_un));

    auto bytes_received = recvfrom(socket_fd, receive_buffer, buffer_size, 0, (struct sockaddr *) &(server_address), &address_length);
    receive_buffer[bytes_received] = '\0';

    source_id = atoi(receive_buffer);

    std::cout << "asgard:ir: remote source: " << source_id << std::endl;

    // Register the button actuator
    nbytes = snprintf(write_buffer, buffer_size, "REG_ACTUATOR %d %s", source_id, "ir_button_1");
    sendto(socket_fd, write_buffer, nbytes, 0, (struct sockaddr *) &server_address, sizeof(struct sockaddr_un));

    bytes_received = recvfrom(socket_fd, receive_buffer, buffer_size, 0, (struct sockaddr *) &(server_address), &address_length);
    receive_buffer[bytes_received] = '\0';

    button_actuator_id = atoi(receive_buffer);

    std::cout << "asgard:ir: remote button actuator: " << button_actuator_id << std::endl;

    //Read the default LIRC config
    struct lirc_config* config;
    if(lirc_readconfig(NULL,&config,NULL)==0){
        char* code;

        //Do stuff while LIRC socket is open  0=open  -1=closed.
        while(lirc_nextcode(&code)==0){
            //If code = NULL, nothing was returned from LIRC socket
            if(code){
                //Send code further
                ir_received(code);

                //Need to free up code before the next loop
                free(code);
            }
        }

        //Frees the data structures associated with config.
        lirc_freeconfig(config);
    } else {
        std::cout << "asgard:ir: Failed to read LIRC config" << std::endl;
    }

    stop();

    return 0;
}
