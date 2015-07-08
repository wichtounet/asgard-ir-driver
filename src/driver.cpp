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

//Buffer
char write_buffer[4096];
char receive_buffer[4096];

void ir_received(int socket_fd, char* raw_code, int actuator){
    std::string full_code(raw_code);

    auto code_end = full_code.find(' ');
    std::string code(full_code.begin(), full_code.begin() + code_end);

    auto repeat_end = full_code.find(' ', code_end + 1);
    std::string repeat(full_code.begin() + code_end + 1, full_code.begin() + repeat_end);

    auto key_end = full_code.find(' ', repeat_end + 1);
    std::string key(full_code.begin() + repeat_end + 1, full_code.begin() + key_end);

    std::cout << "asgard:ir: Received: " << code << ":" << repeat << ":" << key << std::endl;

    //Send the event to the server
    auto nbytes = snprintf(write_buffer, 4096, "EVENT %d %s", actuator, key.c_str());
    write(socket_fd, write_buffer, nbytes);
}

} //End of anonymous namespace

int main(){
    //Initiate LIRC. Exit on failure
    char lirc_name[] = "lirc";
    if(lirc_init(lirc_name, 1) == -1){
        std::cout << "asgard:ir: Failed to init LIRC" << std::endl;
        return 1;
    }

    //Open the socket
    auto socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0){
        std::cout << "asgard:ir: socket() failed" << std::endl;
        return 1;
    }

    //Init the address
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, "/tmp/asgard_socket");

    //Connect to the server
    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        std::cout << "asgard:ir: connect() failed" << std::endl;
        return 1;
    }

    //Register the actuator
    auto nbytes = snprintf(write_write_buffer, 4096, "REG_ACTUATOR ir_remote");
    write(socket_fd, write_write_buffer, nbytes);

    //Get the response from the server
    nbytes = read(socket_fd, receive_buffer, 4096);

    if(!nbytes){
        std::cout << "asgard:ir: failed to register actuator" << std::endl;
        return 1;
    }

    receive_buffer[nbytes] = 0;

    //Parse the actuator id
    int actuator = atoi(receive_buffer);
    std::cout << "remote actuator: " << actuator << std::endl;

    //Read the default LIRC config
    struct lirc_config* config;
    if(lirc_readconfig(NULL,&config,NULL)==0){
        char* code;

        //Do stuff while LIRC socket is open  0=open  -1=closed.
        while(lirc_nextcode(&code)==0){
            //If code = NULL, nothing was returned from LIRC socket
            if(code){
                //Send code further
                ir_received(socket_fd, code, actuator);

                //Need to free up code before the next loop
                free(code);
            }
        }

        //Frees the data structures associated with config.
        lirc_freeconfig(config);
    } else {
        std::cout << "asgard:ir: Failed to read LIRC config" << std::endl;
    }

    //Closes LIRC
    lirc_deinit();

    //Close the socket
    close(socket_fd);

    return 0;
}
