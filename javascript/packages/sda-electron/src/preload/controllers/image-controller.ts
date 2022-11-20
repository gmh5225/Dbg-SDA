import { invokerFactory } from '../utils';
import { ImageController, Image } from '../../api/image';
import { ObjectId } from '../../api/common';

const invoke = invokerFactory('Image');

const ImageControllerImpl: ImageController = {
  getImage: (id: ObjectId) => invoke('getImage', id),

  createImage: (contextId: ObjectId, name: string, analyserName: string, pathToImage: string) =>
    invoke('createImage', contextId, name, analyserName, pathToImage),

  changeImage: (dto: Image) => invoke('changeImage', dto),
};

export default ImageControllerImpl;
