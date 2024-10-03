#!/bin/sh
cd /home/navn/workspace/rust/rrun
gcc -Ofast rrun.c -o ~/bin/rrun
RUSTFLAGS="-C prefer-dynamic" cargo build --release
# cargo build --release
cp -u target/release/deps/*.so ~/bin/lib/rust
cp -u target/release/deps/*.rlib ~/bin/lib/rust
cp -u ~/nonssd/rust/rustup/toolchains/nightly-x86_64-unknown-linux-gnu/lib/libstd-*.so ~/bin/lib/rust
