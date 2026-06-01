#ifndef __ERROR_H_KAL890
#define __ERROR_H_KAL890

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define handle_error(msg)                                                                          \
    do                                                                                             \
    {                                                                                              \
        perror(msg);                                                                               \
        exit(EXIT_FAILURE);                                                                        \
    } while (0)

#endif /* __ERROR_H_KAL890 */
