#ifndef _EVAS_PATH_H
#define _EVAS_PATH_H

/**
 * @brief Joins two path components with the appropriate separator.
 * @param path The first part of the path.
 * @param end The second part of the path to append.
 * @return A newly allocated string containing the joined path.
 * @see evas_file_path_join() in evas_path.c for more details.
 */
char      *evas_file_path_join         (const char *path, const char *end);

/**
 * @brief Checks if a file or directory exists at the given path.
 * @param path The path to check.
 * @return 1 if the path exists, 0 otherwise.
 */
int        evas_file_path_exists       (const char *path);

/**
 * @brief Checks if the given path points to a regular file.
 * @param path The path to check.
 * @return 1 if the path is a regular file, 0 otherwise.
 */
int        evas_file_path_is_file      (const char *path);

/**
 * @brief Checks if the given path points to a directory.
 * @param path The path to check.
 * @return 1 if the path is a directory, 0 otherwise.
 */
int        evas_file_path_is_dir       (const char *path);

/**
 * @brief Lists files and directories within a given path.
 * @param path The directory path to list.
 * @param match A shell wildcard pattern to filter results (e.g., "*.txt").
 *              If @c NULL, all entries are returned.
 * @param match_case If 0, matching is case-insensitive.
 * @return An Eina_List of strings (file/directory names).
 * @see evas_file_path_list() in evas_path.c for more details on return value and memory management.
 */
Eina_List *evas_file_path_list         (char *path, const char *match, int match_case);

/**
 * @brief Gets the last modification time of a file.
 * @param file The path to the file.
 * @return The modification time as a DATA64. Returns 0 on error.
 * @see evas_file_modified_time() in evas_path.c for more details.
 */
DATA64     evas_file_modified_time     (const char *file);

/**
 * @brief Resolves a file path to its absolute form.
 * @param file The file path to resolve.
 * @return A newly allocated string containing the resolved path.
 * @see evas_file_path_resolve() in evas_path.c for current implementation details.
 */
char      *evas_file_path_resolve      (const char *file);


#endif /* _EVAS_PATH_H */
