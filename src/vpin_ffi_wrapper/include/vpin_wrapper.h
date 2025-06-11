#ifndef VPIN_WRAPPER_H
#define VPIN_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

char* get_vpx_table_info_as_json(const char* vpx_file_path);

// Function to free strings allocated by Rust
void free_rust_string(char* s);

#ifdef __cplusplus
}
#endif

#endif // VPIN_WRAPPER_H