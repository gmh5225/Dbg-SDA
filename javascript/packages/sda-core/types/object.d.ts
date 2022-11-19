import { IIdentifiable, ISerializable, Hash } from "./utils";

export abstract class Object implements IIdentifiable, ISerializable {
    readonly hashId: Hash;
    readonly className: string;
    readonly id: string;
    
    setTemporary(temporary: boolean): void;

    serialize(): object;

    deserialize(data: object): void;

    static Get(hashId: Hash): Object;
}

export abstract class ContextObject extends Object {
    name: string;
    comment: string;
}