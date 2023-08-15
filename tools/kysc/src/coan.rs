use std::process::Command;

#[derive(Clone, Debug)]
pub(crate) enum CoanMode {
    Define,
    Undefine,
}

pub(crate) struct Coan;

impl Coan {
    pub fn execute(path: String, mode: CoanMode, defs: Vec<String>) {
        let mut cmd = Command::new("coan");
        cmd.arg("source");
        for def in defs {
            cmd.arg(format!("{}{}", match mode {
                CoanMode::Define => "-D",
                CoanMode::Undefine => "-U",
            }, def));
        }
        cmd.arg("-r");
        cmd.arg(path);
        cmd.output().expect("failed to execute process");
    }
}