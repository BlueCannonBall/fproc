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

    // Run the test file with an ID of 0 (first process)
    console.log(await fproc.run("node run.js", 0));

    // Get all running processes
    console.log(await fproc.list());

    // Stop the first process
    console.log(await fproc.stop(0));

    // Restart the first process
    console.log(await fproc.restart(0));

    // Delete the first process
    console.log(await fproc.delete(0));

    // Closes the session with fproc (If you don't do this Node will run until you do)
    fproc.finish();
});
```

Look at src/fproc.js for all functions and full documentation

# How to install fproc

Go to https://github.com/BlueCannonBall/fproc/blob/main/README.md for instructions
