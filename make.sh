#!/bin/sh
RUSTFLAGS="-C prefer-dynamic" cargo build --release
cp target/release/deps/*.so ~/bin/lib/rust
