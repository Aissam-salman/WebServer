use std::env;
use std::io::{self, Read};

fn main() {
    let method = env::var("REQUEST_METHOD").unwrap_or_default();
    let content_length = env::var("CONTENT_LENGTH").unwrap_or_default();
    let server_protocol = env::var("SERVER_PROTOCOL").unwrap_or_default();
    let server_name = env::var("SERVER_NAME").unwrap_or_default();
    let server_port = env::var("SERVER_PORT").unwrap_or_default();
    let gateway_interface = env::var("GATEWAY_INTERFACE").unwrap_or_default();
    let server_software = env::var("SERVER_SOFTWARE").unwrap_or_default();
    let remote_addr = env::var("REMOTE_ADDR").unwrap_or_default();
    let remote_port = env::var("REMOTE_PORT").unwrap_or_default();
    let path_info = env::var("PATH_INFO").unwrap_or_default();
    let path_translated = env::var("PATH_TRANSLATED").unwrap_or_default();
    let query_string = env::var("QUERY_STRING").unwrap_or_default();
    let content_type = env::var("CONTENT_TYPE").unwrap_or_default();
    let script_name = env::var("SCRIPT_NAME").unwrap_or_default();
    let script_filename = env::var("SCRIPT_FILENAME").unwrap_or_default();

    println!("------- [DEBUG] ---------------------------");
    println!("REQUEST_METHOD={method}");
    println!("CONTENT_LENGTH={content_length}");
    println!("SERVER_PROTOCOL={server_protocol}");
    println!("SERVER_NAME={server_name}");
    println!("SERVER_PORT={server_port}");
    println!("GATEWAY_INTERFACE={gateway_interface}");
    println!("SERVER_SOFTWARE={server_software}");
    println!("REMOTE_ADDR={remote_addr}");
    println!("REMOTE_PORT={remote_port}");
    println!("PATH_INFO={path_info}");
    println!("PATH_TRANSLATED={path_translated}");
    println!("QUERY_STRING={query_string}");
    println!("CONTENT_TYPE={content_type}");
    println!("SCRIPT_NAME={script_name}");
    println!("SCRIPT_FILENAME={script_filename}");
    println!("----------- END DEBUG -------------------");

    if method == "POST" {
        let mut body = String::new();
        io::stdin().read_to_string(&mut body).unwrap();
        println!("Body: {body}");
    } else if method == "GET" {
        println!("[DEBUG] => GET")
    }
}
