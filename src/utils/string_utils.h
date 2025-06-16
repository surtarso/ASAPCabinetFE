/**
 * @file string_utils.h
 * @brief Defines the StringUtils class for utility functions in ASAPCabinetFE.
 *
 * This header provides the StringUtils class, which contains utility methods for
 * string normalization, version comparison, date parsing, and JSON array joining.
 * These methods are designed to support VPS data processing in ASAPCabinetFE,
 * particularly for metadata matching and enrichment. The functionality can be
 * extended with configUI for custom normalization rules in the future.
 */

#ifndef STRING_UTILS_H
#define STRING_UTILS_H // Header guard to prevent multiple inclusions

#include <string> // For std::string to handle text data
#include <map>
#include <nlohmann/json.hpp> // For nlohmann::json to process JSON arrays

/**
 * @class StringUtils
 * @brief Utility class for VPS-related string and data processing in ASAPCabinetFE.
 *
 * This class provides static methods to normalize strings, compare versions,
 * extract years from dates, and join JSON arrays into strings. These utilities
 * are used for processing VPS metadata, ensuring consistency in matching and
 * enrichment tasks. The methods are stateless (const) and can be extended via
 * configUI for user-defined normalization or formatting rules.
 */
class StringUtils {
public:
    /**
     * @brief Normalizes a string for strict comparison.
     *
     * Converts the input string to lowercase and removes all non-alphanumeric
     * characters, producing a simplified string for strict matching (e.g., in
     * metadata comparison). This method can be customized via configUI for
     * different normalization rules.
     *
     * @param input The input string to normalize.
     * @return The normalized string (lowercase, alphanumeric only).
     */
    std::string normalizeString(const std::string& input) const;

    /**
     * @brief Normalizes a string with less aggressive rules.
     *
     * Converts the input string to lowercase, removes specific punctuation
     * (e.g., underscores, dashes), preserves spaces and parentheses, collapses
     * multiple spaces into one, and trims whitespace. This method is suitable
     * for less strict matching while retaining some structure.
     *
     * @param input The input string to normalize.
     * @return The normalized string (lowercase, with preserved spaces/parentheses).
     */
    std::string normalizeStringLessAggressive(const std::string& input) const;

    /**
     * @brief Normalizes a version string for comparison.
     *
     * Trims whitespace, replaces commas with dots, and extracts the first part
     * before a dash if it matches a numeric pattern (e.g., "1.2-beta" becomes "1.2").
     * This method ensures consistent version formatting for comparison.
     *
     * @param version The version string to normalize.
     * @return The normalized version string (trimmed, numeric format).
     */
    std::string normalizeVersion(const std::string& version) const;

    /**
     * @brief Compares two version strings.
     *
     * Normalizes the version strings using normalizeVersion, then compares them
     * numerically if possible (e.g., "1.2" > "1.1"). Falls back to lexicographical
     * comparison if numeric conversion fails. Returns true if v1 is greater than v2.
     *
     * @param v1 The first version string to compare.
     * @param v2 The second version string to compare.
     * @return True if v1 is greater than v2, false otherwise.
     */
    bool isVersionGreaterThan(const std::string& v1, const std::string& v2) const;

    /**
     * @brief Extracts the year from a date string.
     *
     * Parses the date string to extract a four-digit year, supporting formats like
     * "DD.MM.YYYY" (e.g., "01.01.2023") or standalone "YYYY" (e.g., "2023").
     * Returns an empty string if no year is found.
     *
     * @param dateString The date string to parse.
     * @return The extracted year as a string, or empty if not found.
     */
    std::string extractYearFromDate(const std::string& dateString) const;

    /**
     * @brief Joins a JSON array of strings into a single string.
     *
     * Concatenates all string elements in the JSON array using the specified
     * delimiter, skipping non-string elements with a debug log. Returns an empty
     * string if the array is empty or contains no valid strings.
     *
     * @param array The JSON array containing strings to join.
     * @param delimiter The string to use as a separator between elements.
     * @return The joined string, or empty if no valid elements.
     */
    std::string join(const nlohmann::json& array, const std::string& delimiter) const;

    /**
     * @brief Safely extracts a string value from a JSON object.
     *
     * Retrieves the string value associated with the given key from a JSON object.
     * If the key is not found or the value is not a string, it returns a default value.
     * Handles null values by returning the default.
     *
     * @param j The JSON object to extract from.
     * @param key The key of the string value to extract.
     * @param defaultValue The default string value to return if extraction fails.
     * @return The extracted string value or the default value.
     */
    std::string safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue) const;

    /**
     * @brief Cleans a string by trimming whitespace and collapsing multiple spaces.
     *
     * Removes leading and trailing whitespace characters from the input string and
     * replaces any sequence of multiple internal spaces with a single space.
     *
     * @param input The string to clean.
     * @return The cleaned string.
     */
    std::string cleanString(const std::string& input) const;

    size_t levenshteinDistance(const std::string& s1, const std::string& s2) const;
    std::string toLower(const std::string& str) const;
    std::string extractCleanTitle(const std::string& input) const;
    
};

#endif // STRING_UTILS_H