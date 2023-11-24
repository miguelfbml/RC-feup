#include <stdio.h>
#include <stdlib.h>





int main(int argc, char *argv[]) {


    if (argc != 2) {
        printf("Invalid Input.\n");
        printf("Usage: ./download ftp://<user>:<password>@<host>/<url-path>\n");
        exit(-1);
    }


    //"ftp://[<username>:<password>@]<ftp.example.com>/<path/to/file.txt>"

    char *url = argv[1];

    printf("%s\n", url);

    char username[100], password[100], domain[100], path[200];

    int result = sscanf(url, "ftp://%99[^:]:%99[^@]@%99[^/]/%99s", username, password, domain, path);

    if (result == 4) {
        printf("Username: %s\n", username);
        printf("Password: %s\n", password);
        printf("Domain: %s\n", domain);
        printf("Path: %s\n", path);
    } else {
        printf("Failed to parse the URL.\n");
    }

    return 0;
}


