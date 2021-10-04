# Fprocjs

A JS framework for the fastest process manager written in C++ and Rust, Fproc.

# How to install fprocjs

```sh
npm install fprocjs
```

# Quick setup

```js
const FPROC = require("fprocjs");

const fproc = new FPROC(async () => {
    // Run when ready

    // Get the list of process
    await fproc.list();

    // Stop the first process
    await fproc.stop(0);

    // Restart the first process
    await fproc.restart(0);

    // Delete the first process
    await fproc.delete(0);

    // Close the connection to fproc (If you don't do this Node will run until you do)
    fproc.closeFproc();
});
```

Look at src/fproc.js for all functions and full documentation

# How to install fproc

Go to https://github.com/BlueCannonBall/fproc/blob/main/README.md for instructions
