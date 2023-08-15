use std::path::Path;
use coan::{Coan, CoanMode};
use fsu::FsUtils;
use clap::{Parser, Subcommand};
use std::fs;

/// Simple coan wrapper to remove definitions from a directory
#[derive(Parser, Debug)]
#[command(name = "kysc", version)]
struct Args {
    #[clap(subcommand)]
    subcmd: SubCommands
}

#[derive(Debug, Subcommand)]
pub enum SubCommands {
    /// Preprocess macro definitions
    Define {
        /// Path to the directory to remove the definitions from
        path: String,
        /// List of definitions to remove
        defines: Vec<String>,
    },
    /// Delete a time entry
    Undefine {
        /// Path to the directory to remove the definitions from
        path: String,
        /// List of definitions to remove
        defines: Vec<String>,
    }
}

mod fsu;
mod coan;

fn run_coan(path: String, mode: CoanMode, defs: Vec<String>) {
    let allowed = vec!["h", "c", "hpp", "cpp"];
    let _rs = FsUtils::visit_dirs(Path::new(path.as_str()), &|entry| {
        let path = entry.path();
        let ext = path.extension();
        if ext.is_none() {
            return;
        }
        let file = fs::canonicalize(&path).unwrap();
        if !allowed.contains(&ext.unwrap().to_str().unwrap()) {
            return;
        }
        let can = file.to_str().unwrap();
        Coan::execute(String::from(can), mode.clone(), defs.clone());
        println!("{}", can);
    });
}

fn main() {
    let args = Args::parse();
    match args.subcmd {
        SubCommands::Define { path, defines } => {
            run_coan(path, CoanMode::Define, defines);
        },
        SubCommands::Undefine { path, defines } => {
            run_coan(path, CoanMode::Undefine, defines);
        }
    }
}
