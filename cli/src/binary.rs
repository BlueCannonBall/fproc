#![allow(dead_code)]
use std::io::Write;

use std::io::Cursor;
use std::io::Read;


pub struct StreamPeerBuffer {
    pub cursor: Cursor<Vec<u8>>
}
impl StreamPeerBuffer {
    pub fn new () -> StreamPeerBuffer {
            //static mut data :Vec<u8> = vec![];
            //let cursor = Cursor::new(&mut data);
        StreamPeerBuffer {
            cursor: Cursor::new(Vec::new())
        }
    }

    pub fn put_u8 (&mut self, value:u8) {
        self.cursor.get_mut().write(&[value]).expect("Put error");
    }

    pub fn put_u16 (&mut self, value:u16) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_u32 (&mut self, value:u32) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_u64 (&mut self, value:u64) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_8 (&mut self, value:i8) {
        self.cursor.get_mut().write(&[value as u8]).expect("Put error");
    }

    pub fn put_16 (&mut self, value:i16) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_32 (&mut self, value:i32) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_64 (&mut self, value:i64) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_float (&mut self, value:f32) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    pub fn put_double (&mut self, value:f64) {
        self.cursor.get_mut().write(&value.to_be_bytes()).expect("Put error");
    }

    // minecraft encodings
    pub fn put_varint_mc (&mut self, mut value:i32) {
        while value != 0 {
            let mut temp:u8 = (value & 0b01111111) as u8;
            value = value >> 7;
            if value != 0 {
                temp = temp | 0b10000000;
            }
            self.put_u8(temp);
        }
    }

    pub fn get_varint_mc (&mut self) -> i32 {
        let mut numRead:i32 = 0;
        let mut result:i32 = 0;
        let mut read:u8 = 255;

        while (read & 0b10000000) != 0 {
            read = self.get_u8();
            let value:i32 = (read & 0b01111111) as i32;
            result = result | (value << (7 * numRead));

            numRead += 1;
            if numRead > 5 {
                panic! ("VarInt is too big");
            }
        }
        result
    }
    pub fn put_varlong (&mut self, mut value:i64) {
        while value != 0 {
            let mut temp:u8 = (value & 0b01111111) as u8;
            value = value >> 7;
            if value != 1 {
                temp = temp | 0b10000000;
            }
            self.put_u8(temp);
        }
    }
    pub fn get_varlong (&mut self) -> i64 {
        let mut numread:i32 = 0;
        let mut result:i64 = 0;
        let mut read:u8 = 255;
        while (read & 0b10000000) != 0 {
            read = self.get_u8();
            let value = (read & 0b01111111) as i64;
            result = result | (value << (7 * numread));

            numread += 1;
            if numread > 10 {
                panic!("Varlong is too big")
            }
        }

        result
    }

    pub fn put_varint_utf8 (&mut self, value:String) {
        let utf8 = value.as_bytes();
        self.put_varint_mc(utf8.len() as i32);
        for byte in utf8.iter() {
            self.put_u8(*byte);
        }
    }

    pub fn get_varint_utf8 (&mut self) -> String {
        let length = self.get_varint_mc();
        let mut buf:Vec<u8> = vec![];

        for _ in 0..length {
            buf.push(self.get_u8());
        }

        String::from_utf8(buf).expect("Invalid utf8 in buf")
    }

    // end of minecraft encodings

    pub fn get_u8 (&mut self) -> u8 {
        let mut res:[u8;1] = [0;1];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        res[0]
    }

    pub fn get_u16 (&mut self) -> u16 {
        let mut res:[u8;2] = [0;2];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u16::from_be_bytes(res)
    }

    pub fn get_u32 (&mut self) -> u32 {
        let mut res:[u8;4] = [0;4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u32::from_be_bytes(res)
    }

    pub fn get_u64 (&mut self) -> u64 {
        let mut res:[u8;8] = [0;8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u64::from_be_bytes(res)
    }

    pub fn get_8 (&mut self) -> i8 {
        let mut res:[u8;1] = [0;1];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        res[0] as i8
    }

    pub fn get_16 (&mut self) -> i16 {
        let mut res:[u8;2] = [0;2];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i16::from_be_bytes(res)
    }

    pub fn get_32 (&mut self) -> i32 {
        let mut res:[u8;4] = [0;4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i32::from_be_bytes(res)
    }

    pub fn get_64 (&mut self) -> i64 {
        let mut res:[u8;8] = [0;8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i64::from_be_bytes(res)
    }

    pub fn get_float (&mut self) -> f32 {
        let mut res:[u8;4] = [0;4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        f32::from_be_bytes(res)
    }

    pub fn get_double (&mut self) -> f64 {
        let mut res:[u8;8] = [0;8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        f64::from_be_bytes(res)
    }

    // strings
    pub fn put_utf8 (&mut self, value:String) {
        let utf8 = value.as_bytes();
        self.put_u16(utf8.len() as u16);
        for byte in utf8.iter() {
            self.put_u8(*byte);
        }
    }

    pub fn get_utf8 (&mut self) -> String {
        let length = self.get_u16();
        let mut buf:Vec<u8> = vec![];

        for _ in 0..length {
            buf.push(self.get_u8());
        }

        String::from_utf8(buf).expect("Invalid utf8 in buf")
    }

    pub fn set_data_array (&mut self, new_data:Vec<u8>) {
        let cursor = Cursor::new(Vec::new());
        //drop(self.cursor);
        self.cursor = cursor;
        let reference = self.cursor.get_mut();
        for byte in new_data.iter() {
            reference.push(*byte);
        }

    }

}