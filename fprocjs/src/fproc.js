const net = require("net");
const os = require("os");
const headers = require("./headers.js");
const HSPB = require("./reader.js");

class FprocError extends Error {
    constructor(message) {
        super(message);
    }
}

class FprocElement {
    /**
     * A FprocList element
     * @param {number} id Process ID
     * @param {string} name Process name
     * @param {number} pid Process PID
     * @param {boolean} running If the process is running
     * @param {number} restarts The amount of restarts for the process
     */
    constructor(id, name, pid, running, restarts) {
        this.id = id;
        this.name = name;
        this.pid = pid;
        this.running = running;
        this.restarts = restarts;
    }
}

class FprocList {
    /**
     * Data structure for the Fproc list
     * @param {number} processAmount The amount of processes
     * @param {Array<FprocElement>} processes The process's info
     */
    constructor(processAmount, processes) {
        this.processAmount = processAmount;
        this.processes = processes;
    }
}

class Fproc {
    /**
     * FPROC Framework
     * @param {Function} callback Callback to when fproc is loaded fully
     * @returns {void}
     * @author Sopur
     */
    constructor(callback = function () {}) {
        if (typeof callback !== "function") {
            throw new TypeError("Fproc constructor called with an invalid function instance");
        }
        this.STATES = {
            OPEN: 0,
            OPENING: 1,
            CLOSED: 2,
        };
        this.state = this.STATES.OPENING;
        this.fproc = net.createConnection({ path: os.userInfo().homedir + "/.fproc.sock" }, () => {
            this.state = this.STATES.OPEN;
            callback();
        });

        this.fproc.on("data", (data) => {
            this._ondone(this._asyncRequest(data.buffer));
        });
        this.fproc.on("error", (err) => {
            throw new FprocError("Fproc encountered an error: " + err.message);
        });
    }

    /**
     * Runs a command with the fproc daemon
     * @param {string} command Command to be run
     * @param {number} customID A custom ID for the command (If left alone, it will automatically set the ID to one that isn't in use)
     * @param {string} directory The target directory for the command to be ran in (Default is this module's directory)
     * @param {Object} env The environment variables to be passed down to the process
     * @returns {Promise<boolean>} Failed or not
     */
    async run(command, customID = undefined, directory = __dirname, env = process.env) {
        if (typeof command !== "string" || command.length <= 0) {
            throw new FprocError("The command argument must be a string with a content length of at least 1");
        }
        if ((typeof customID !== "undefined" && typeof customID !== "number") || customID < 0) {
            throw new FprocError("The custom ID must be either undefined (meaning unset/auto) or a valid number with a value of at least 0");
        }
        if (typeof directory !== "string") {
            throw new FprocError("The custom directory must be a string");
        }
        if (typeof env !== "object") {
            throw new FprocError("The env must be an object");
        }

        this.checkErrors();
        const writer = new HSPB.HolyStreamPeerBufferWriter(JSON.stringify(env).length + 100);
        writer.u8 = headers.Run;
        writer.string = command;
        writer.u8 = Number(customID !== undefined);
        if (customID !== undefined) {
            writer.u32 = customID;
        }
        writer.u32 = Object.keys(env).length;
        for (const key in env) {
            writer.string = key;
            writer.string = env[key].toString();
        }
        writer.string = directory;
        this.fproc.write(writer.buffer);
        return await this._awaitAsyncRequest(this._errorCheckResponse);
    }

    /**
     * Deletes a process from the Fproc daemon
     * @param {number} id Process ID
     * @returns {Promise<boolean>} Failed or not
     */
    async delete(id) {
        return await this._idSendRequest(id, headers.Delete);
    }

    /**
     * Stops a process in the Fproc daemon
     * @param {number} id Process ID
     * @returns {Promise<boolean>} Failed or not
     */
    async stop(id) {
        return await this._idSendRequest(id, headers.Stop);
    }

    /**
     * Gets the list of processes running with Fproc
     * @returns {Promise<FprocList>} Processes info
     */
    async list() {
        this.checkErrors();
        const writer = new HSPB.HolyStreamPeerBufferWriter();
        writer.u8 = headers.List;
        this.fproc.write(writer.buffer);
        return await this._awaitAsyncRequest(this._list);
    }

    async restart(id) {
        return await this._idSendRequest(id, headers.Start);
    }

    /**
     * Closes the Fproc socket
     * @returns {void}
     * @readonly
     */
    finish() {
        this.state = this.STATES.CLOSED;
        this.fproc.destroy();
    }

    /**
     * Checks for errors with the fproc socket or states
     * @returns {void}
     * @private
     */
    checkErrors() {
        if (this.state !== this.STATES.OPEN)
            throw new FprocError("Tried to access fproc while fproc isn't open (Use the constructor as a callback when fproc is open)");
    }

    /**
     * @private
     */
    _asyncRequest() {}

    /**
     * @private
     */
    _ondone() {}

    /**
     * Does an ID send request (Like the start, stop, and restart requests)
     * @param {number} id ID of choice
     * @param {number} header Packet header
     * @returns {Promise<boolean>} Failed
     * @private
     */
    async _idSendRequest(id, header) {
        if (typeof id !== "number" || isNaN(id)) {
            throw new Error('Parameter "ID" needs to be a valid number');
        }
        this.checkErrors();
        const writer = new HSPB.HolyStreamPeerBufferWriter();
        writer.u8 = header;
        writer.u32 = id;
        this.fproc.write(writer.buffer);
        return await this._awaitAsyncRequest(this._errorCheckResponse);
    }

    /**
     * Formats the list of processes from Fproc
     * @param {ArrayBuffer} buffer Response from Fproc daemon
     * @returns {Object} Process amount and info about each process
     * @private
     */
    _list(buffer) {
        const reader = new HSPB.HolyStreamPeerBufferReader(buffer);
        let listed = {
            processAmount: reader.u32,
            processes: [],
        };
        for (let i = 0; i < listed.processAmount; i++) {
            listed.processes.push({
                id: reader.u32,
                name: reader.string,
                pid: reader.u32,
                running: reader.u8 !== 0,
                restarts: reader.u32,
            });
        }
        return listed;
    }

    /**
     * Checks if the error response was 1 to throw an error
     * @param {ArrayBuffer} buffer Response from Fproc daemon
     * @returns {boolean}
     * @private
     */
    _errorCheckResponse(buffer) {
        const reader = new HSPB.HolyStreamPeerBufferReader(buffer);
        if (reader.u8 === 1) {
            throw new FprocError("Fproc sent an error: " + reader.string);
            return false;
        } else {
            return true;
        }
    }

    /**
     * Waits for a message to run a callback
     * @param {Function} callback Callback for when fproc sends a response
     * @returns {Promise<any>} The formatted fproc info
     * @private
     */
    _awaitAsyncRequest(callback) {
        this._asyncRequest = callback;
        return new Promise((res) => {
            this._ondone = res;
        });
    }
}

module.exports = Fproc;
