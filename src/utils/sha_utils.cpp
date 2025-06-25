#include "sha_utils.h"
#include "log/logging.h"
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>

std::string normalize_line_endings(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    bool last_was_cr = false;
    for (char c : input) {
        if (c == '\r') {
            last_was_cr = true;
            continue;
        }
        if (c == '\n') {
            result += "\r\n";
            last_was_cr = false;
            continue;
        }
        if (last_was_cr) {
            result += "\r\n";
            last_was_cr = false;
        }
        result += c;
    }
    if (last_was_cr) {
        result += "\r\n";
    }
    return result;
}

std::string calculate_string_sha256(const std::string& input) {
    std::string normalized = normalize_line_endings(input);

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        LOG_ERROR("ShaUtils: Failed to create EVP_MD_CTX");
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to initialize SHA256 digest");
        return "";
    }

    if (EVP_DigestUpdate(mdctx, normalized.c_str(), normalized.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to update SHA256 digest");
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to finalize SHA256 digest");
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string compute_file_sha256(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("ShaUtils: Failed to open file for hashing: " << filename);
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::string normalized = normalize_line_endings(content);

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        LOG_ERROR("ShaUtils: Failed to create EVP_MD_CTX");
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to initialize SHA256 digest");
        return "";
    }

    if (EVP_DigestUpdate(mdctx, normalized.c_str(), normalized.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to update SHA256 digest");
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        LOG_ERROR("ShaUtils: Failed to finalize SHA256 digest");
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}