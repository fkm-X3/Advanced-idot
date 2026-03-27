mod lexer;
mod parser;
mod semantic;
mod codegen;
mod stdlib;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "idotc")]
#[command(about = "Advanced-idot compiler", long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Build the project
    Build {
        /// Input file to compile
        file: String,
        /// Output file name
        #[arg(short, long)]
        output: Option<String>,
    },
    /// Run the project (compile and execute)
    Run {
        /// Input file to compile and run
        file: String,
    },
    /// Check the project without codegen
    Check {
        /// Input file to check
        file: String,
    },
}

fn main() {
    let cli = Cli::parse();

    match cli.command {
        Commands::Build { file, output } => {
            println!("Building {}...", file);
            if let Some(out) = output {
                println!("Output: {}", out);
            }
            // TODO: Implement compilation
        }
        Commands::Run { file } => {
            println!("Running {}...", file);
            // TODO: Implement compile + execute
        }
        Commands::Check { file } => {
            println!("Checking {}...", file);
            // TODO: Implement syntax/type checking
        }
    }
}

