const FPROC = require("./src/fproc.js");

const fproc = new FPROC(async () => {
    // Run when ready

    // Get the list of process
    console.log(await fproc.list());

    // Stop the first process
    console.log(await fproc.stop(0));

    // Restart the first process
    console.log(await fproc.restart(0));

    // Delete the first process
    console.log(await fproc.delete(0));

    // Close the connection to fproc (If you don't do this Node will run until you do)
    fproc.closeFproc();
});
