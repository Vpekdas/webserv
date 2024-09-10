use std::env;

fn main() {
	println!("Content-Type: text/html\r");
	println!("\r");

	env::vars().for_each(|(key, value)| println!("{key}: {value}<br />"));
}
