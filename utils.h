#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "constants.h"

uint8_t sprint(const char *_Message);
bool has_ok_file_extension(const char *_FileName, const char *_ExtensionName);




/**
 * A safe print with dynamic memory allocation.
 * 
 * @param _Message The actual message you want to print.
 * @return 0 on success, ERROR_SPRINT on fail memory allocation.
 */
uint8_t sprint(const char *_Message) {
    size_t length = strlen(_Message);
    char *str = (char*)malloc(length + 1);
    if (str == NULL) {
        errno = ENOMEM;
        return ERROR_SPRINT;
    }
    snprintf(str, length + 1, "%s", _Message);
    fprintf(stderr, "%s", str);
    free(str);
    return 0;
}


/**
 * Check if the file has the right extension
 * 
 * @param _FileName The name of the file.
 * @param _ExtensionName The extension to be checked.
 * @return true if match, false if error or don't match.
 */
bool has_ok_file_extension(const char *_FileName, const char *_ExtensionName) {
    // > not sure though if I should set hardcoded errno here
    const char *ext = strchr(_FileName, '.');
    
    if (ext == NULL) {
        return false;
    }

    if (strcmp(ext, _ExtensionName) != 0) {
        return false;
    }

    return true;
}
