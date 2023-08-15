use std::io;
use std::fs::{self, DirEntry};
use std::path::Path;

pub(crate) struct FsUtils;

impl FsUtils {
    // one possible implementation of walking a directory only visiting files
    pub(crate) fn visit_dirs(dir: &Path, cb: &dyn Fn(&DirEntry)) -> io::Result<()> {
        if dir.is_dir() {
            for entry in fs::read_dir(dir)? {
                let entry = entry?;
                let path = entry.path();
                if path.is_dir() {
                    Self::visit_dirs(&path, cb)?;
                } else {
                    cb(&entry);
                }
            }
        }
        Ok(())
    }
}