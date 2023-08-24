// Section: 2

// Member 1
// Name: Kinjal Doshi
// Student ID: 110099228

// Member 2
// Name: Chintal Raval
// Student ID: 110096546

// Required header for storing socket structures
#include <netinet/in.h>
// basic I/O lib
#include <stdio.h>
#include <stdlib.h>
// For edge point creation
#include <sys/socket.h> 
#include <sys/types.h>

// For transforming network structure in binary
#include <arpa/inet.h>

// other required libraries
#include <string.h> 
#include <stdbool.h>
#include <unistd.h>  

// Variable define for max valid extensions in the targzf command
#define MAX_EXTENSIONS 4

// Flag for checking option is there in the command or not
bool unzip_option = false;

// Function to extract tar.gz file
void extract_tar_gz(const char *tar_gz_filename) {
    // Variable for tar command to extract tar.gz file
    char tar_command[1024];
    
    snprintf(tar_command, sizeof(tar_command), "tar -xzf %s", tar_gz_filename);

    // system() checks tar -xzf command executed properly or not
    int result = system(tar_command);
    if (result == 0) {
        // Successfully executed and extracted the file
        printf("The file has been extracted successfully.\n");
    } else {
        // Failed to extract the file
        printf("Error: There was an issue while extracting the file.\n");
    }
}

// Fn to process the response from the server
void process_response(int client_sd) {
    printf("Waiting for Response from Server...\n");
    // Used to read the response
    char buffer[50000];
    // set all the values of it to NULL
    memset(buffer, 0, sizeof(buffer));

    // Get first response
    recv(client_sd, buffer, 50000, 0);
    if(strcmp(buffer,"TAR_FILE_FLAG") == 0){
        // Received flag for tar, set all values of buffer to 0 for reading next response
        memset(buffer, 0, sizeof(buffer));
        
        // Receive another response, that would be size of file
        recv(client_sd, buffer, 50000, 0);
        // Convert received string to long
        int bytes_of_tar_file = atol(buffer);

        // Allocate required memory
        char *buffer_for_tar_file = (char *)malloc(bytes_of_tar_file);
        if (buffer_for_tar_file == NULL) {
            fprintf(stderr,  "Error: There was a problem allocating memory.\n");
            return;
        }
        
        // Name for creating trar file at client side
        char *target_f_name = "temp.tar.gz";
        FILE *rec_tar_file = fopen(target_f_name, "wb");
        if (!rec_tar_file) {
            fprintf(stderr, "Error: Unable to open the file for writing.\n");
            return;
        }
       
        // Next step is to receive tar file. It can be huge so we need to receive and process it iteratively
        ssize_t bytes_received = 0;
        size_t total_bytes_received = 0;
        while (total_bytes_received < bytes_of_tar_file) {
            // receive bytes in a loop
            bytes_received = recv(client_sd, buffer_for_tar_file + total_bytes_received, bytes_of_tar_file - total_bytes_received, 0);
            if (bytes_received <= 0) {
                if(bytes_received == 0){
                    fprintf(stderr, "Error: The connection has been terminated by the server side.\n");
                } else {
                    fprintf(stderr, "Error: An error occurred while receiving data.\n");
                }
                // Something failed, exit
                fclose(rec_tar_file);
                free(buffer_for_tar_file);
                return;
            }
            total_bytes_received += bytes_received;
        }
        // All okay in receiving, Null terminate buffer
        buffer_for_tar_file[bytes_of_tar_file] = '\0';
        // Write received binary data
        fwrite(buffer_for_tar_file, 1, bytes_of_tar_file, rec_tar_file);
        fclose(rec_tar_file);
        printf("The compressed file has been successfully received.\n");

        // Check if extraction is needed or not
        if(unzip_option){
            extract_tar_gz(target_f_name);
        }
    }
    else if(strcmp(buffer, "MESSAGE_FLAG") == 0){
        // received a message flag, set buffer to 0
        memset(buffer, 0, sizeof(buffer));
        // receive message
        recv(client_sd, buffer, 50000, 0);
        // Null terminate message
        buffer[sizeof(buffer) -1 ] = '\0';
        // print message
        printf("%s\n", buffer);
    }
    else {
        fprintf(stderr, "Error: The received response is not compatible or expected.\n");
    }    
    printf("------------------------------------------------------------------------\n");
    return;
}

// Function to send the command entered by the user to the server
void send_command_to_server(int client_sd, const char *command) {
    // Check send is successfull or not
    if (send(client_sd, command, strlen(command), 0) < 0) {
        // Error while sending the command to the server
        fprintf(stderr, "send() failed\n");
        exit(4);
    }
}

// Function to check the date given in argument is in valid date format
bool is_valid_date(const char *date) {
    // Checking length of date string
    if (strlen(date) != 10) {
        return false;
    }

    // Parse year, month, and day from the date string
    int year, month, day;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) {
        return false;
    }

    // Checking the parsed year, month and day value is in proper range
    if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }

    // Valid date
    return true;
}

// Function to check the range of the two dates i.e date1<=date2
bool is_valid_date_range(const char *date1, const char *date2) {
    if (!is_valid_date(date1) || !is_valid_date(date2)) {
        // Not a valid date format of the dates
        return false; 
    }

    // Compare date1 and date2
    int year1, month1, day1, year2, month2, day2;
    sscanf(date1, "%d-%d-%d", &year1, &month1, &day1);
    sscanf(date2, "%d-%d-%d", &year2, &month2, &day2);

    if (year1 > year2) {
        // date2 is before date1
        return false; 
    } else if (year1 == year2 && month1 > month2) {
        // date2 is before date1
        return false; 
    } else if (year1 == year2 && month1 == month2 && day1 > day2) {
        // date2 is before date1
        return false;
    }

     // Valid date range
    return true;
}

// Function to check the extensions provided in targzf command are valid
bool validate_extensions(const char *extension_list) {
    // Count variable to make sure max 4 extensions are provided by the user
    int count = 0;
    
    // char array for storing extensions
    char *extensions[MAX_EXTENSIONS];
    
    // Creating copy of the extension list in the command to manipulate it
    char extension_list_copy[50];
    strcpy(extension_list_copy, extension_list);

    // Tokenize the extensions based on the space delimiter
    char *token = strtok(extension_list_copy, " ");
    
    while (token != NULL) {
        // Check if the extension is already in the extensions array
        for (int i = 0; i < count; i++) {
            if (strcmp(extensions[i], token) == 0) {
                // Duplicate extension found, return false
                return false;
            }
        }

        // Store extension in the extension array
        if(strcmp(token, "-u") !=0 ){
            extensions[count] = token;
            // Increment the count
            count++;
        }
        token = strtok(NULL, " ");
    }

    // Ensure the count of extensions is not more than 4
    if (count > MAX_EXTENSIONS ) {
        return false;
    }else if(count == 0){
        // If no extensions provided 
        return false;
    }

    // Valid extension list
    return true;
}

// Function to validate fgets command in client before sending it to server
bool handle_fgets_command(int client_sd, const char *command) {
    // Creating copy of command to manipulate it
    char command_copy[1000];
    strcpy(command_copy, command);

    // Extract file names from the command
    char *file_names = strchr(command_copy, ' ') + 1;

    // Check if at least one file name provided
    if (strlen(file_names) == 0) {
        printf("Error: At least one filename is required.\n");
        return false;
    }

    // Spliting file names based on space delimiter
    char *token = strtok(file_names, " ");
    // Varibale to count total number of file names in the command
    int file_count = 0;
    while (token != NULL) {
        // Check if each filename has an extension
        // Checking based on period(.) delimiter
        char *extension = strchr(token, '.');
        if (extension == NULL) {
            // If no extension provided to the file
            printf("Error: Filename '%s' must have an extension.\n", token);
            return false;
        }

        // Incrementing the count as per the file names in the command
        file_count++;
        // Checking if more than 4 names in the command
        if (file_count > 4) {
            printf("Error: Only up to 4 files are allowed.\n");
            return false;
        }

        token = strtok(NULL, " ");
    }

    // Send command to the server
    send_command_to_server(client_sd,command);
    return true;
}

// Function to validate tarfgetz command in client before sending it to server
bool handle_tarfgetz_command(int client_sd, const char *command) {
    // Extract the arguments i.e size, size2 and option from the command
    char size1_str[10];
    char size2_str[10];
    char opt_given[5];
    int num_args = sscanf(command, "tarfgetz %s %s %s", size1_str, size2_str, opt_given);

    // Checking total number of arguments in the command
    if (num_args != 2 && num_args != 3) {
        printf("Error: Invalid number of arguments. Usage: tarfgetz size1 size2 [-u]\n");
        return false;
    }
    
    // Check if the size arguments are valid integers
    int size1, size2;
    if (sscanf(size1_str, "%d", &size1) != 1 || sscanf(size2_str, "%d", &size2) != 1) {
        printf("Error: Invalid size arguments. Usage: tarfgetz size1 size2 [-u]\n");
        return false;
    }

    // Check size1 and size2 are valid i.e size1 < = size2 (size1>= 0 and size2>=0)
    if (size1 < 0 || size2 < 0 || size1 > size2) {
        // Not valid sizes
        printf("Error: Invalid size range.\n");
        return false;
    }

    // Check if the -u option is provided
    unzip_option = false;
    if (num_args == 3) {
        // Check correct option is provided in the command
        char option[3];
        sscanf(command, "tarfgetz %s %s %s", size1_str, size2_str, option);
        // Compare the option with -u
        if (strcmp(option, "-u") == 0) {
            unzip_option = true;
        } else {
            // Other than -u is provided in the command
            printf("Error: Invalid option. Usage: tarfgetz size1 size2 [-u]\n");
            return false;
        }
    }

    // Send command to the server
    send_command_to_server(client_sd,command);
    return true;
}

// Function to validate filesrch command in client before sending it to server
bool handle_filesrch_command(int client_sd, const char *command) {
    // Extract the filename from the command
    char filename[100];
    if (sscanf(command, "filesrch %99s", filename) != 1) {
        // Filename not provided in the command
        printf("Error: Invalid arguments. Usage: filesrch filename\n");
        return false;
    }

    // Check if the filename has an extension
    // Filename separated from extension based on period(.) delimeter
    char *extension = strchr(filename, '.');
    if (extension == NULL) {
        // No extension given to the filename
        printf("Error: Filename '%s' must have an extension.\n", filename);
        return false;
    }

    // Send command to the server
    send_command_to_server(client_sd,command);
    return true;
}

// Function to validate targzf command in the client before sending it to server
bool handle_targzf_command(int client_sd, const char *command) {
    // Copying command to command_copy to manipulate the command
    char command_copy[1000];
    strcpy(command_copy,command);

    // Extract extension list and -u option if present
    char *extension_list = strchr(command_copy, ' ') + 1;
    char *u_option = strstr(command_copy, " -u");
    unzip_option = false;
    // Check the extracted option is not NULL
    if (u_option != NULL) {
        unzip_option = true;
    }

    // Validate all the extracted extensions from the command
    if (validate_extensions(extension_list)) {
        // Extensions valid
        // Send the command to the server
        send_command_to_server(client_sd,command);
        return true;
    } else {
        // Error not a valid extensions or more than 4 extensions are in the command
        printf("Error: Maximum %d extensions allowed.\n", MAX_EXTENSIONS);
        return false;
    }
}

// Function to validate getdirf command in the client before sending it to server
bool handle_getdirf_command(int client_sd, const char *command) {
    // Extract date1, date2 and options from the command
    char date1_str[11];
    char date2_str[11];
    char opt_given[5];
    int num_args = sscanf(command, "getdirf %s %s %s", date1_str, date2_str, opt_given);

    // Check if the correct number of arguments is provided
    if (num_args != 2 && num_args != 3) {
        // Proper argument not provided
        printf("Error: Invalid number of arguments. Usage: getdirf date1 date2 [-u]\n");
        return false;
    }

    // Validate both date arguments
    if (!is_valid_date_range(date1_str,date2_str)) {
        printf("Error: The provided dates are not valid.\n");
        return false;
    }

    // Check if the -u option is provided
    unzip_option = false;
    if (num_args == 3) {
        char option[3];
        sscanf(command, "getdirf %s %s %s", date1_str, date2_str, option);
        // Check only -u is given as option in the command
        if (strcmp(option, "-u") == 0) {
            unzip_option = true;
        } else {
            // Show error otherwise
            printf("Error: Invalid option. Usage: getdirf date1 date2 [-u]\n");
            return false;
        }
    }

    // Send command to the server
    send_command_to_server(client_sd, command);
    return true;
}

int main(int argc, char *argv[]){
    // Numbers in human readable format
    int client_sd, port_number;
    socklen_t len;
    // required structure
    struct sockaddr_in server_add;

    if(argc != 3){
        // Not a valid CLI 
        printf("Please check structure:%s <IP> <Port#>\n",argv[0]);
        exit(0);
    }

    // create socket at client side
    if ((client_sd=socket(AF_INET,SOCK_STREAM,0))<0){ //socket()
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    // Build strucutre for socket
    server_add.sin_family = AF_INET; //Internet 
    sscanf(argv[2], "%d", &port_number);
    server_add.sin_port = htons((uint16_t)port_number);//Port number

    // Convert address in binary
    if(inet_pton(AF_INET, argv[1],&server_add.sin_addr) < 0){
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    // connect to server, if failed, immediately exit
    if(connect(client_sd, (struct sockaddr *) &server_add,sizeof(server_add))<0){//Connect()
        fprintf(stderr, "connect() failed, exiting\n");
        exit(3);
    }

    // Array for user entered command
    char user_input[1000];
    while(1){
        // First initialized to false - predicting no option there in the command
        unzip_option = false;

        printf("\nEnter a command: ");
        
        // fgets to read input from the user i.e STDIN and store into user_input array
        fgets(user_input, sizeof(user_input), stdin);
        user_input[strcspn(user_input, "\n")] = '\0'; // Remove newline

        // Variable that keeps check of the command entered by the user are valid or not
        // Predicting command entered is not valid
        bool command_success = false;
    
        // Check command entered is "quit"
        if (strncmp(user_input, "quit", 4) == 0) {
            // Send "quit" command to server
            send_command_to_server(client_sd,user_input);
            break;
        } else if (strncmp(user_input, "fgets", 5) == 0) {
            // Command entered is "fgets"
            // Check the entered command is valid and store true or false to the command_success variable
            command_success = handle_fgets_command(client_sd, user_input);
        } else if (strncmp(user_input, "tarfgetz", 8) == 0) {
            // Command entered is "tarfgetz"
            // Check the entered command is valid and store true or false to the command_success variable
            command_success = handle_tarfgetz_command(client_sd, user_input);
        } else if (strncmp(user_input, "filesrch", 8) == 0) {
            // Command entered is "filesrch"
            // Check the entered command is valid and store true or false to the command_success variable
            command_success = handle_filesrch_command(client_sd, user_input);
        } else if (strncmp(user_input, "targzf", 6) == 0) {
            // Command entered is "targzf"
            // Check the entered command is valid and store true or false to the command_success variable
            command_success = handle_targzf_command(client_sd, user_input);
        } else if (strncmp(user_input, "getdirf", 7) == 0) {
            // Command entered is "getdirf"
            // Check the entered command is valid and store true or false to the command_success variable
            command_success = handle_getdirf_command(client_sd, user_input);
        } else {
            // Command entered that has not been mentioned above
            printf("Error: Plesae enter a valid command\n");
            command_success = false;
        }

        // Receive and print response from the server
        if(command_success){
            // Method to handle the received response from the server
            process_response(client_sd);
        }
    }
    close(client_sd);
    exit(0);
}
