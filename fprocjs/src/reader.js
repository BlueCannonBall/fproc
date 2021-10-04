class HolySteamPeerBufferReader {
    constructor(buffer) {
        this.arrayView = new DataView(buffer);
        this.offset = 0;
    }

    get u32() {
        const data = this.arrayView.getUint32(this.offset);
        this.offset += 4;
        return data;
    }

    get u16() {
        const data = this.arrayView.getUint16(this.offset);
        this.offset += 2;
        return data;
    }

    get u8() {
        const data = this.arrayView.getUint8(this.offset);
        this.offset += 1;
        return data;
    }

    get string() {
        const length = this.u16;
        let string = "";
        for (let i = 0; i < length; i++) {
            string += String.fromCharCode(this.u8);
        }
        return string;
    }
}

class HolySteamPeerBufferWriter {
    constructor() {
        this.view = new DataView(new ArrayBuffer(1024));
        this.offset = 0;
    }

    set u8(value) {
        this.view.setUint8(this.offset, value);
        this.offset += 1;
    }

    set u32(value) {
        this.view.setUint32(this.offset, value);
        this.offset += 1;
    }

    set string(string) {
        this.view.setUint16(string.length);
        for (const char of string) {
            this.view.setUint8(char.charCodeAt());
        }
    }

    get buffer() {
        return new Uint8Array(this.view.buffer);
    }
}

module.exports = {
    HolySteamPeerBufferReader,
    HolySteamPeerBufferWriter,
};
