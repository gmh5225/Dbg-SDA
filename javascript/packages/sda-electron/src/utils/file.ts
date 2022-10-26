import { readFile, writeFile, access, constants } from "fs";
import rimraf from 'rimraf';

export function loadJSON<T>(file: string): Promise<T> {
    return new Promise((resolve, reject) => {
        readFile(file, (err, data) => {
            if (err) {
                reject(err);
            } else {
                resolve(JSON.parse(data.toString()));
            }
        });
    });
}

export function saveJSON<T>(file: string, data: T): Promise<void> {
    return new Promise((resolve, reject) => {
        writeFile(file, JSON.stringify(data), (err) => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });
}

export function doesFileExist(file: string): Promise<boolean> {
    return new Promise((resolve, reject) => {
        access(file, constants.F_OK, err => {
            resolve(!err);
        });
    });
}

// Delete a file or directory recursively
export function deleteFile(path: string): Promise<void> {
    return new Promise((resolve, reject) => {
        rimraf(path, err => {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
    });
}