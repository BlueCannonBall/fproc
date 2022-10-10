#![allow(dead_code)]
use std::io::{Cursor, Read, Write};

pub struct StreamPeerBuffer {
    pub cursor: Cursor<Vec<u8>>,
}

impl StreamPeerBuffer {
    pub fn new() -> StreamPeerBuffer {
        StreamPeerBuffer {
            cursor: Cursor::new(Vec::new()),
        }
    }

    pub fn put_u8(&mut self, value: u8) {
        self.cursor.get_mut().write(&[value]).expect("Put error");
    }

    pub fn put_u16(&mut self, value: u16) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_u32(&mut self, value: u32) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_u64(&mut self, value: u64) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_8(&mut self, value: i8) {
        self.cursor
            .get_mut()
            .write(&[value as u8])
            .expect("Put error");
    }

    pub fn put_16(&mut self, value: i16) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_32(&mut self, value: i32) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_64(&mut self, value: i64) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_float(&mut self, value: f32) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn put_double(&mut self, value: f64) {
        self.cursor
            .get_mut()
            .write(&value.to_be_bytes())
            .expect("Put error");
    }

    pub fn get_u8(&mut self) -> u8 {
        let mut res: [u8; 1] = [0; 1];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        res[0]
    }

    pub fn get_u16(&mut self) -> u16 {
        let mut res: [u8; 2] = [0; 2];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u16::from_be_bytes(res)
    }

    pub fn get_u32(&mut self) -> u32 {
        let mut res: [u8; 4] = [0; 4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u32::from_be_bytes(res)
    }

    pub fn get_u64(&mut self) -> u64 {
        let mut res: [u8; 8] = [0; 8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        u64::from_be_bytes(res)
    }

    pub fn get_8(&mut self) -> i8 {
        let mut res: [u8; 1] = [0; 1];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        res[0] as i8
    }

    pub fn get_16(&mut self) -> i16 {
        let mut res: [u8; 2] = [0; 2];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i16::from_be_bytes(res)
    }

    pub fn get_32(&mut self) -> i32 {
        let mut res: [u8; 4] = [0; 4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i32::from_be_bytes(res)
    }

    pub fn get_64(&mut self) -> i64 {
        let mut res: [u8; 8] = [0; 8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        i64::from_be_bytes(res)
    }

    pub fn get_float(&mut self) -> f32 {
        let mut res: [u8; 4] = [0; 4];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        f32::from_be_bytes(res)
    }

    pub fn get_double(&mut self) -> f64 {
        let mut res: [u8; 8] = [0; 8];
        self.cursor.read(&mut res).expect("Failed to read bytes");
        f64::from_be_bytes(res)
    }

    // strings
    pub fn put_utf8(&mut self, value: String) {
        let utf8 = value.as_bytes();
        self.put_u16(utf8.len() as u16);
        for byte in utf8.iter() {
            self.put_u8(*byte);
        }
    }

    pub fn get_utf8(&mut self) -> String {
        let length = self.get_u16();
        let mut buf: Vec<u8> = vec![];

        for _ in 0..length {
            buf.push(self.get_u8());
        }

        String::from_utf8(buf).expect("Invalid utf8 in buf")
    }

    pub fn set_data_array(&mut self, new_data: Vec<u8>) {
        let cursor = Cursor::new(Vec::new());
        //drop(self.cursor);
        self.cursor = cursor;
        let reference = self.cursor.get_mut();
        for byte in new_data.iter() {
            reference.push(*byte);
        }
    }
}
