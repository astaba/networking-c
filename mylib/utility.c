/* mylib/utility.c */

#include "omniplat.h"

/**
 * @brief Strips the directory path from a file path, leaving only the filename.
 *
 * @param file A pointer to a `char*` holding the full file path.
 * On return, this `char*` will point to the basename (filename).
 *
 * @note Original string is not modified; only the pointer is advanced.
 * Handles both Windows ('\') and Unix ('/') separators.
 */
void basename(char **file) {
  // Strip directory path
  char *separator = NULL;
#if defined(_WIN32)
  separator = strrchr(*file, '\\');
#else
  separator = strrchr(*file, '/');
#endif
  if (separator)
    *file = ++separator;
}
