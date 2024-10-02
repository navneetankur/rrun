#!/bin/sh
cd /home/navn/workspace/rust/rrun
gcc -Ofast rrun.c -o ~/bin/rrun
RUSTFLAGS="-C prefer-dynamic" cargo build --release
cp -vu target/release/deps/*.so ~/bin/lib/rust
cp -vu ~/nonssd/rust/rustup/toolchains/nightly-x86_64-unknown-linux-gnu/lib/libstd-*.so ~/bin/lib/rust
