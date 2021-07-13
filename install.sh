#!/bin/sh

echo "Installing fproc..."

cp ./daemon/fprocd /usr/local/bin
cp ./cli/target/release/fproc /usr/local/bin
strip /usr/local/bin/fproc

echo "Successfully installed fproc!"
