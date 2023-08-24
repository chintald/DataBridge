// Section: 2

// Member 1
// Name: Kinjal Doshi
// Student ID: 110099228

// Member 2
// Name: Chintal Raval
// Student ID: 110096546

// basic libraries
#include<stdio.h>
#include<stdlib.h>
// for socket sys call
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h> 
#include<time.h> 
#include<stdbool.h>

// to convert human readble format of address to binary
#include<arpa/inet.h> 
#include<netinet/in.h> 
#include<unistd.h>
#include<sys/stat.h>


#define MAX_FOUND_FILES 2000
#define MAX_PATH_LENGTH 256

// Varibale to store path of created tar file
char *path_to_created_tar_file;


// Function to get current working directory
bool get_current_working_dir(){
    // Buffer to hold the directory path
    char current_wd[1024]; 

    // getcwd() function used to get current working directory and storing it in current_wd buffer
    if (getcwd(current_wd, sizeof(current_wd)) != NULL) {
        char tar_file_path[1024];
        // Created path for temp.tar.gz file and stored to the path_to_created_tar_file varibale
        snprintf(tar_file_path, sizeof(current_wd), "%s/%s", current_wd, "temp.tar.gz");
        // Copy the modified path
        path_to_created_tar_file = strdup(tar_file_path); 
        return true;
    } else {
        // Error retrieving current working directory
        fprintf(stderr, "Could not get the current working directory\n");
    }
    return false;
}

// Function to send response to the client from the server
void send_response(int connection_sd, bool send_tar_file, char *path_of_tar_file, char *response_message){   
    // bool for tar file
    if(send_tar_file){
        // Open the temp.tar.gz file i.e binary file in read mode
        FILE *file = fopen(path_of_tar_file, "rb");
        if (!file) {
            // Error opening the file
            perror("Error: The requested file could not be open.\n");
            exit(1);
        }
        // set pointer to end to get size of file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        // got file size, set pointer to beginning
        fseek(file, 0, SEEK_SET);

        // Dynalically allocate memory
        char *file_data = (char *)malloc(file_size);
        // read and close file
        fread(file_data, 1, file_size, file);   
        fclose(file);
        // send flag to client
        send(connection_sd, "TAR_FILE_FLAG", 13, 0);
        sleep(1);
        // send size to client
        char str_to_send[50];
        snprintf(str_to_send, sizeof(str_to_send), "%ld", file_size);
    	send(connection_sd, str_to_send, sizeof(str_to_send), 0);
        sleep(1);
    	// send data to client
        ssize_t bytes_sent = send(connection_sd, file_data, file_size, 0);
        if (bytes_sent == -1) {
            fprintf(stderr, "Error: An issue occurred while sending the data.\n");
            exit(1);
        }
        // free memory
        free(file_data);
    }
    // not a tar file
    else if(!send_tar_file){
        // Sending message to the client
        send(connection_sd, "MESSAGE_FLAG", 12, 0);
        sleep(2);
        send(connection_sd, response_message, strlen(response_message), 0);    
    }
    else{
        fprintf(stderr, "Please check function parameters for sending response.\n");
        exit(1);
    }
    printf("Message/tar.gz file was successfully transmitted.\n");
    printf("------------------------------------------------------------------------\n\n");
}

// Function to remove the temp.tar.gz file
void remove_temp_tar_gz() {
    // Use the find command to check if temp.tar.gz exists
    char find_command[100];
    snprintf(find_command, sizeof(find_command), "find . -maxdepth 1 -name temp.tar.gz");

    // Execute the find command and check its result
    int result = system(find_command);
    if (result == 0) {
        unlink("temp.tar.gz");
    }
}

// Function to handle the fgets command implementation
void handle_fgets_command_from_client(int connection_sd, const char *command){
    // Remove already existed temp.tar.gz file before executing the command
    remove_temp_tar_gz();
    
    // Creating copy of the command to manipulate it
    char command_copy[1000];
    strcpy(command_copy, command);

    // Remove the trailing newline character from the command
    command_copy[strcspn(command_copy, "\n")] = '\0';

    // Extract filenames from the command
    char *file_names = strchr(command_copy, ' ') + 1;

    // Tokenize the filenames based on space delimiter
    char *token = strtok(file_names, " ");

    // Variable to check any file found in the server with the filename given in the command
    // First intialized it with false
    bool found = false;
    // Buffer to store a space-separated list of found files
    char found_files[2000] = ""; 
    while (token != NULL) {
        // Use the find command to search for the file in $HOME
        char find_command[2000];
        snprintf(find_command, sizeof(find_command), "find \"%s\" -name \"%s\"", getenv("HOME"), token);

        // Open a pipe to capture the output of the find command
        FILE *fp = popen(find_command, "r");
        if (fp != NULL) {
            char result_line[256];
            if (fgets(result_line, sizeof(result_line), fp) != NULL) {
                // Remove newline characters from the result_line
                result_line[strcspn(result_line, "\n")] = '\0';
                
                // Found the file path with the specified filename
                if (strstr(result_line, token) != NULL) {
                    // File found in the system 
                    found = true;

                    // Append the full path of the found file to the list, enclosed in quotes
                    strcat(found_files, "\"");
                    strcat(found_files, result_line);
                    strcat(found_files, "\" ");
                } else {
                    // Filename in the command is not present in the server
                    printf("File %s not found\n", token);
                }
            }
            // Close the pipe
            pclose(fp);
        } else {
            // Error in find command
            printf("Error executing find command\n");
        }

        // Move to the next token 
        token = strtok(NULL, " ");
    }

    // Create a tar.gz file of the found files
    if (found) {
        printf("Generating and compressing data into tar.gz file...\n");
        // Buffer to store the tar command
        char tar_command[2000];
        snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz %s", found_files);
        
        // Execute the tar command and store the result in tar_result varibale
        int tar_result = system(tar_command);
        if (tar_result == 0) {
            // Tar command successfully executed
            printf("The tar.gz file has been successfully generated.\n");
            // Send the tar.gz file to the client via send_response function
            send_response(connection_sd, true, path_to_created_tar_file, "");
        } else {
            // Error creating tar.gz file
            printf("There was an issue while creating the tar.gz file.\n");
        }
    } else {
        // If not files found with the specified name than send message to the client via send_response function
        send_response(connection_sd, false, "", "No file found.");
    }
}

// Function to handle tarfgetz command implementation
void handle_tarfgetz_command_from_client(int connection_sd, const char *command) {
    // Delete the already existed tar.gz file from the server before executing the command
    remove_temp_tar_gz();
    
    // Extract the size1 and size2 from the command
    char size1_str[10];
    char size2_str[10];
    int num_args = sscanf(command, "tarfgetz %s %s", size1_str, size2_str);

    // Check if the size arguments are valid integers
    int size1, size2;
    if (sscanf(size1_str, "%d", &size1) != 1 || sscanf(size2_str, "%d", &size2) != 1) {
        // Not a valid size arguments
        printf("Error: Invalid size arguments. Usage: tarfgetz size1 size2 [-u]\n");
        return;
    }

    // Create a command to find the files
    char find_command[2000];
    // Store find command in the find_command buffer
    snprintf(find_command, sizeof(find_command),
             "find \"%s\" -type f -size +%dc -size -%dc",
             getenv("HOME"), size1, size2);

    // Open a pipe to capture the output of the find command
    FILE *find_pipe = popen(find_command, "r");
    if (find_pipe == NULL) {
        // Error in pipe
        perror("popen");
        return;
    }

    // Buffer for reading find command output
    char response[MAX_PATH_LENGTH]; 

    // Variable to check any file found in the server between the specified size range
    // First intialized it with false
    bool files_found = false;

    // Create a temporary file to store the list of files
    // Open that file in write mode
    FILE *file_list_file = fopen("file_list.txt", "w");
    if (file_list_file == NULL) {
        // Error opening file
        perror("fopen");
        pclose(find_pipe);
        return;
    }

    // Write the list of found files between specified size range to the temporary file
    while (fgets(response, sizeof(response), find_pipe) != NULL) {
        // Remove new line character from the file path
        response[strcspn(response, "\n")] = '\0';
        fprintf(file_list_file, "%s\n", response);
        // Files found so variable to true
        files_found = true;
    }

    // Close the temporary created file after writing all the found files path to it
    fclose(file_list_file);
    // Close the pipe
    pclose(find_pipe);

    if (files_found) {
        // Generating tar.gz file of all the found files
        printf("Generating and compressing data into tar.gz file...\n");
        // Build the tar command using the temporary file
        // Store that tar command in tar_command buffer
        char tar_command[2000];
        snprintf(tar_command, sizeof(tar_command),
                 "tar czf temp.tar.gz -T file_list.txt");

        // Execute the tar command and store the result in result varibale
        int result = system(tar_command);
        if (result == 0) {
            // Tar command executed successfully
            printf("The tar.gz file has been successfully generated.\n");
            // Send tar file to the client from server via send_response function
            send_response(connection_sd, true, path_to_created_tar_file, "");
        } else {
            // Error creating tar.gz file
            printf("There was an issue while creating the tar.gz file.\n");
        }
    } else {
        // No files found within the specified size range so send message to the client
        send_response(connection_sd, false, "", "No file found.");
    }
}

// Function to handle filesrch command implementation
void handle_filesrch_command_from_client(int connection_sd, const char *command){
    // Buffer to store the extracted file name from the filesrch command
    char filename[100];
    if (sscanf(command, "filesrch %99s", filename) != 1) {
        // No file name provided in the command
        printf("Error: Invalid arguments. Usage: filesrch filename\n");
        return;
    }

    // Store the find command in the find_command buffer
    char find_command[200];
    snprintf(find_command, sizeof(find_command), "find \"%s\" -name \"%s\"", getenv("HOME"), filename);

    // Pipe to store the output to the find command
    FILE *find_pipe = popen(find_command, "r");
    if (find_pipe == NULL) {
        // Error creating pipe
        perror("popen");
        return;
    }

    // Buffer for reading find command output
    char response[200];
    if (fgets(response, sizeof(response), find_pipe) != NULL) {
        // Extract the found file path
        char *found_file_path = strtok(response, "\n");

        // Get file size and date created
        struct stat file_stat;
        if (stat(found_file_path, &file_stat) == 0) {
            printf("Retrieving file information.\n");
            
            // Creation/Modification time of the file
            time_t creation_time = file_stat.st_ctime;

            // Buffer to store size of the file
            char size_str[20];
            snprintf(size_str, sizeof(size_str), "%ld", file_stat.st_size);

            // Creation/Modification date of the file
            char date_str[30];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", localtime(&creation_time));

            // Buffer to store the constructed response of the file information
            char file_found_response[10000];
            // Construct the response string
            snprintf(file_found_response, sizeof(file_found_response), "File: %s\nSize: %s bytes\nDate Created: %s",
                     found_file_path, size_str, date_str);

            // Send the file information to the client via send_response function from server
            send_response(connection_sd, false, "", file_found_response);
        } else {
            // Error retrieving information of the file
            printf("Unable to obtain file information.\n");
        }
    } else {
        // No file with the specified name found so send the message to the client
        send_response(connection_sd, false, "", "Files not found.");
    }

    // Close the pipe
    pclose(find_pipe);
}

// Function to handle targzf command implementation in the server
void handle_targzf_command_from_client(int connection_sd, const char *command) {
    // Remove the already existed tar.gz file from the server before executing the command
    remove_temp_tar_gz();

    // Extract the extension list from the command
    char extension_list[100];
    sscanf(command, "targzf %[A-Za-z .]s", extension_list);

    // Tokenize the extension list based on space delimiter
    char *token = strtok(extension_list, " ");
    // Variable to store the count of total extensions in the command
    int ext_count = 0;

    // Buffer to store the extensions
    char find_extensions[1024];
    // Loop through each extension and build the extensions buffer
    while (token != NULL && ext_count < 4) {
        if (ext_count == 0) {
            sprintf(find_extensions, "\\( -name \"*.%s\"", token);
        } else {
            strcat(find_extensions, " -o -name \"*.");
            strcat(find_extensions, token);
            strcat(find_extensions, "\"");
        }
        token = strtok(NULL, " ");
        ext_count++;
    }

    strcat(find_extensions, " \\)");

    // Build the find command to check for files
    char command_check_files[1024 * 2];
    sprintf(command_check_files, "find ~ -type f %s", find_extensions);

    // Open a pipe to capture the output of the find command
    FILE *find_pipe = popen(command_check_files, "r");
    if (find_pipe == NULL) {
        // Error opening the pipe
        perror("popen");
        return;
    }

    // Response buffer to read the output of the find command
    char response[MAX_PATH_LENGTH];
    if (fgets(response, sizeof(response), find_pipe) == NULL) {
        // No files found with the specified extension
        // Send message to the client from the server
        send_response(connection_sd, false, "", "No file found.");
        // Close pipe
        pclose(find_pipe);
        return;
    }

    // Close pipe
    pclose(find_pipe);

    // Build the final find and tar command
    printf("Generating and compressing data into tar.gz file...\n");
    // Buffer to store the find and tar command 
    char command_find_tar[1024 * 4];
    sprintf(command_find_tar, "find ~ -type f %s -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null", find_extensions);

    // Execute the command and store the result of the command execution in the result varibale
    int result = system(command_find_tar);

    if (result == 0) {
        // Tar file created successfully of all the found files with the extension provided in the command
        printf("The tar.gz file has been successfully generated.\n");
        // Send the tar.gz file to the client via send_response function from the server
        send_response(connection_sd, true, path_to_created_tar_file, "");
    } else {
        // Error creating tar.gz file
        printf("There was an issue while creating the tar.gz file.\n");
    }
}

// Function to handle getdirf command implementation in the server
void handle_getdirf_command_from_client(int connection_sd, const char *command) {
    // Remove already existed temp.tar.gz file from the server before executing the command
    remove_temp_tar_gz();

    // Extracting date1 and date2 from the command
    char date1_str[20], date2_str[20];
    sscanf(command, "getdirf %19s %19s", date1_str, date2_str);

    // Buffer to store the find command
    char find_command[2000];
    snprintf(find_command, sizeof(find_command),
             "find \"%s\" -type f -newermt \"%s\" ! -newermt \"%s\" -print0",
             getenv("HOME"), date1_str, date2_str);

    // Pipe to store the output of the find command based on date1 and date2
    FILE *find_pipe = popen(find_command, "r");
    if (find_pipe == NULL) {
        // Error opening pipe
        perror("popen");
        return;
    }

    // Buffer for reading find command output
    char response[MAX_PATH_LENGTH]; 

    // Variable to check any file found in the server which is created or modified between the given date range
    // First intialized it with false
    bool files_found = false;

    // fgets command to read output from the pipe
    while ((fgets(response, sizeof(response), find_pipe) != NULL)) {
        // Remove newline characters from the response
        response[strcspn(response, "\n")] = '\0';

        // Check if the file exists and is accessible
        if (access(response, F_OK) == 0) {
            // File exist and is accessible so flag to true
            files_found = true;
        }
    }

    // Close the pipe
    pclose(find_pipe);

    if (files_found) {
        printf("Generating and compressing data into tar.gz file...\n");
        // Buffer to store the find and tar command 
        char tar_command[2000];
        snprintf(tar_command, sizeof(tar_command), "find \"%s\" -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null",
                 getenv("HOME"), date1_str, date2_str);

        // Execute the command and store the result to result variable
        int result = system(tar_command);
        if (result == 0) {
            // Tar file successfully created
            printf("The tar.gz file has been successfully generated.\n");
            // Send the tar.gz file to the client via send_response function from the server
            send_response(connection_sd, true, path_to_created_tar_file, "");
        } else {
            // Creating creating tar.gz file
            printf("There was an issue while creating the tar.gz file.\n");
        }
    } else {
        // No files found between the specified date range so send no file found message to client from server
        send_response(connection_sd, false, "", "No file found.");
    }
}


// Function to process the client requested command implementation
void process_client(int connection_sd) {
    bool continue_loop = true;
    char command[1000];
    while(continue_loop){
        // Receive the command from the client
        ssize_t bytes_received = recv(connection_sd, command, sizeof(command) - 1, 0);
        if(strcmp(command, "") == 0 || strcmp(command, " ") == 0){
            continue_loop = false;
            break;
        }
        
        // Null-terminate the received data
        command[bytes_received] = '\0';

        // Create the copy of the command to able to manipulate the command from the client
        char command_copy[1000];
        strcpy(command_copy, command);

        // Tokenize the command based on space delimiter
        char *command_type = strtok(command_copy, " ");

        if (strcmp(command_type, "fgets") == 0) {
            // Command from the client is "fgets"
            // Fgets command implementation
            handle_fgets_command_from_client(connection_sd, command);
        } else if (strcmp(command_type, "tarfgetz") == 0) {
            // Command from the client is "tarfgetz"
            // tarfgetz command implementation
            handle_tarfgetz_command_from_client(connection_sd, command);
        } else if (strcmp(command_type, "filesrch") == 0) {
            // Command from the client is "filesrch"
            // filesrch command implementation
            handle_filesrch_command_from_client(connection_sd, command);
        } else if (strcmp(command_type, "targzf") == 0) {
            // Command from the client is "targzf"
            // targzf command implementation
            handle_targzf_command_from_client(connection_sd, command);
        } else if (strcmp(command_type, "getdirf") == 0) {
            // Command from the client is "getdirf"
            // getdirf command implementation
            handle_getdirf_command_from_client(connection_sd, command);
        } else if (strcmp(command_type, "quit") == 0) { 
            // Command from the client is "quit"
            continue_loop = false;
            break;
        }
        memset(command, '\0', sizeof(command));
    }

    close(connection_sd);
    exit(0);
}

void handle_client_at_server(int connection_sd){
    int pid = fork();
    if(pid == 0){
        process_client(connection_sd);
    } 
    else if(pid == -1){
        fprintf(stderr, "An error occured while connecting to the client.\n");
    }
}
  

int main(int argc, char *argv[]){
    // variables for structure
    int listen_sd, connection_sd, port_number;
    socklen_t len_of_socket;
    struct sockaddr_in server_add;

    if(argc != 2){
        fprintf(stderr,"Call model: %s <Port#>\n",argv[0]);
        exit(0);
    }

    get_current_working_dir();

    // get a socket descriptor
    if((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Could not create socket\n");
        exit(1);
    }

    // create structure for address
    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = htonl(INADDR_ANY);
    sscanf(argv[1], "%d", &port_number);
    server_add.sin_port = htons((uint16_t)port_number);

    // bind descriptor
    bind(listen_sd, (struct sockaddr *) &server_add,sizeof(server_add));

    // listen for connection
    listen(listen_sd, 15);

    while(1){
        // request received
        connection_sd = accept(listen_sd, (struct sockaddr*) NULL, NULL);
        if(connection_sd != -1){
            // able to accept
            handle_client_at_server(connection_sd);
        }
    }
    close(connection_sd);
}
