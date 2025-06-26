use std::os::raw::c_char;
use std::ffi::{CString, CStr};
use std::path::PathBuf;
use std::panic::catch_unwind;
use serde_json::{json, Value};
use vpin::vpx::open;

#[no_mangle]
pub extern "C" fn get_vpx_table_info_as_json(vpx_file_path: *const c_char) -> *mut c_char {
    if vpx_file_path.is_null() {
        eprintln!("get_vpx_table_info_as_json: Input file path is null.");
        return std::ptr::null_mut();
    }

    let path_str = unsafe {
        match CStr::from_ptr(vpx_file_path).to_str() {
            Ok(s) => s,
            Err(_e) => {
                eprintln!("get_vpx_table_info_as_json: Invalid UTF-8 in path: {}", _e);
                return std::ptr::null_mut();
            }
        }
    };

    let path = PathBuf::from(path_str);
    // eprintln!("get_vpx_table_info_as_json: Processing file '{}'", path_str);

    let result = catch_unwind(|| {
        // eprintln!("get_vpx_table_info_as_json: Opening file '{}'", path_str);
        match vpin::vpx::open(&path) {
            Ok(mut vpx_file) => {
                // eprintln!("get_vpx_table_info_as_json: Reading table info for '{}'", path_str);
                match vpx_file.read_tableinfo() {
                    Ok(table_info) => {
                        // eprintln!("get_vpx_table_info_as_json: Table info read succeeded for '{}'", path_str);
                        let mut json_object = json!({
                            "table_name": table_info.table_name,
                            "author_name": table_info.author_name,
                            "table_blurb": table_info.table_blurb,
                            "table_rules": table_info.table_rules,
                            "author_email": table_info.author_email,
                            "release_date": table_info.release_date,
                            "table_save_rev": table_info.table_save_rev,
                            "table_version": table_info.table_version,
                            "author_website": table_info.author_website,
                            "table_save_date": table_info.table_save_date,
                            "table_description": table_info.table_description,
                        });

                        // eprintln!("get_vpx_table_info_as_json: Building properties for '{}'", path_str);
                        let mut properties_obj = serde_json::Map::new();
                        for (key, value) in table_info.properties {
                            // eprintln!("get_vpx_table_info_as_json: Adding property '{}' = '{}' for '{}'", key, value, path_str);
                            properties_obj.insert(key, Value::String(value));
                        }
                        json_object["properties"] = Value::Object(properties_obj);

                        // eprintln!("get_vpx_table_info_as_json: Serializing JSON for '{}'", path_str);
                        let json_string = match serde_json::to_string(&json_object) {
                            Ok(s) => s,
                            Err(_e) => {
                                eprintln!("get_vpx_table_info_as_json: JSON serialization failed for '{}': {}", path_str, _e);
                                return None;
                            }
                        };

                        // eprintln!("get_vpx_table_info_as_json: Converting to CString for '{}'", path_str);
                        match CString::new(json_string) {
                            Ok(c_string) => {
                                // eprintln!("get_vpx_table_info_as_json: Success for '{}'", path_str);
                                Some(c_string.into_raw())
                            }
                            Err(_e) => {
                                eprintln!("get_vpx_table_info_as_json: CString conversion failed for '{}': {}", path_str, _e);
                                None
                            }
                        }
                    }
                    Err(_e) => {
                        eprintln!("get_vpx_table_info_as_json: Failed to read table info for '{}': {}", path_str, _e);
                        None
                    }
                }
            }
            Err(_e) => {
                eprintln!("get_vpx_table_info_as_json: Failed to open '{}': {}", path_str, _e);
                return None;
            }
        }
    });

    match result {
        Ok(Some(ptr)) => ptr,
        Ok(None) => {
            eprintln!("get_vpx_table_info_as_json: Returning null for '{}'", path_str);
            std::ptr::null_mut()
        }
        Err(_e) => {
            eprintln!("get_vpx_table_info_as_json Panic occurred for '{}'", path_str);
            std::ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn get_vpx_gamedata_code(vpx_file_path: *const c_char) -> *mut c_char {
    // Safety check for null pointer
    if vpx_file_path.is_null() {
        eprintln!("get_vpx_gamedata_code: Input file path is null.");
        return std::ptr::null_mut();
    }

    // Convert C string to Rust string
    let path_str = unsafe {
        match CStr::from_ptr(vpx_file_path).to_str() {
            Ok(s) => s,
            Err(e) => {
                eprintln!("get_vpx_gamedata_code: Invalid UTF-8 in path: {}", e);
                return std::ptr::null_mut();
            }
        }
    };

    let path = PathBuf::from(path_str);

    // Use catch_unwind to handle potential panics
    let result = catch_unwind(|| {
        // Open the VPX file
        match open(&path) {
            Ok(mut vpx_file) => {
                // Read only the GameData stream
                match vpx_file.read_gamedata() {
                    Ok(gamedata) => {
                        let code = gamedata.code.string;
                        match CString::new(code) {
                            Ok(c_string) => Some(c_string.into_raw()),
                            Err(e) => {
                                eprintln!(
                                    "get_vpx_gamedata_code: CString conversion failed for '{}': {}",
                                    path_str, e
                                );
                                None
                            }
                        }
                    }
                    Err(e) => {
                        eprintln!(
                            "get_vpx_gamedata_code: Failed to read gamedata for '{}': {}",
                            path_str, e
                        );
                        None
                    }
                }
            }
            Err(e) => {
                eprintln!(
                    "get_vpx_gamedata_code: Failed to open '{}': {}",
                    path_str, e
                );
                None
            }
        }
    });

    // Handle the result
    match result {
        Ok(Some(ptr)) => ptr,
        Ok(None) => {
            eprintln!("get_vpx_gamedata_code: Returning null for '{}'", path_str);
            std::ptr::null_mut()
        }
        Err(_) => {
            eprintln!("get_vpx_gamedata_code: Panic occurred for '{}'", path_str);
            std::ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn free_rust_string(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        _ = CString::from_raw(s);
    }
}