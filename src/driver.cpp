//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "asgard/driver.hpp"

extern "C" {
#include <lirc/lirc_client.h>
}

namespace {

// Configuration
std::vector<asgard::KeyValue> config;

// The driver connection
asgard::driver_connector driver;

// The remote IDs
int source_id = -1;
int button_actuator_id = -1;

void stop(){
    std::cout << "asgard:ir: stop the driver" << std::endl;

    //Closes LIRC
    lirc_deinit();

    asgard::unregister_actuator(driver, source_id, button_actuator_id);
    asgard::unregister_source(driver, source_id);

    // Unlink the client socket
    unlink(asgard::get_string_value(config, "ir_client_socket_path").c_str());

    // Close the socket
    close(driver.socket_fd);
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

    asgard::send_event(driver, source_id, button_actuator_id, key);
}

} //End of anonymous namespace

int main(){
    //Initiate LIRC. Exit on failure
    char lirc_name[] = "lirc";
    if(lirc_init(lirc_name, 1) == -1){
        std::cout << "asgard:ir: Failed to init LIRC" << std::endl;
        return 1;
    }

    // Load the configuration file
    asgard::load_config(config);

    // Open the connection
    if(!asgard::open_driver_connection(driver, asgard::get_string_value(config, "server_socket_addr").c_str(), asgard::get_int_value(config, "server_socket_port"))){
        return 1;
    }

    //Register signals for "proper" shutdown
    signal(SIGTERM, terminate);
    signal(SIGINT, terminate);

    // Register the source and sensors
    source_id = asgard::register_source(driver, "ir");
    button_actuator_id = asgard::register_actuator(driver, source_id, "ir_button_1");

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
