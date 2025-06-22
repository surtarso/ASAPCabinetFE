#ifndef SHA_UTILS_H
#define SHA_UTILS_H

#include <string>

/**
 * @brief Normalizes line endings in a string to \r\n (CRLF).
 * @param input The input string to normalize.
 * @return The string with line endings normalized to \r\n.
 */
std::string normalize_line_endings(const std::string& input);

/**
 * @brief Computes the SHA-256 hash of a string after normalizing line endings.
 * @param input The input string to hash.
 * @return The SHA-256 hash as a lowercase hexadecimal string, or empty string on error.
 */
std::string calculate_string_sha256(const std::string& input);

/**
 * @brief Computes the SHA-256 hash of a file after normalizing line endings.
 * @param filename The path to the file to hash.
 * @return The SHA-256 hash as a lowercase hexadecimal string, or empty string on error.
 */
std::string compute_file_sha256(const std::string& filename);

#endif // SHA_UTILS_H