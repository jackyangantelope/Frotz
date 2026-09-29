#include <string.h>

char *basename(char *path)
{
    char *slash = strrchr(path, '/');
    char *backslash = strrchr(path, '\\');
    char *last = slash;
    if (backslash != 0 && (last == 0 || backslash > last)) last = backslash;
    return last == 0 ? path : last + 1;
}
