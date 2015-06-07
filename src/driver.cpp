#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include <lirc/lirc_client.h>
}

void ir_received(char* raw_code){
    std::string full_code(raw_code);

    auto code_end = full_code.find(' ');
    std::string code(0, code_end);

    auto repeat_end = full_code.find(' ', code_end + 1);
    std::string repeat(code_end + 1, repeat_end);

    auto key_end = full_code.find(' ', repeat_end + 1);
    std::string key(repeat_end + 1, key_end);

    std::cout << "Code: " << code << std::endl;
    std::cout << "Repeat: " << repeat << std::endl;
    std::cout << "Key: " << key << std::endl;
}

int main(){
    //Initiate LIRC. Exit on failure
    char lirc_name[] = "lirc";
    if(lirc_init(lirc_name, 1) == -1){
        return 1;
    }

    struct lirc_config* config;

    //Read the default LIRC config at /etc/lirc/lircd.conf  This is the config for your remote.
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
    }

    //Closes LIRC
    lirc_deinit();

    return 0;
}
