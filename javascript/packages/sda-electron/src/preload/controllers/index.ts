import { contextBridge } from 'electron';
import EventController from './event-controller';
import WindowController from './window-controller';
import ProjectController from './project-controller';
import PlatformController from './platform-controller';

const initControllers = () => {
    contextBridge.exposeInMainWorld('eventApi', EventController);
    contextBridge.exposeInMainWorld('windowApi', WindowController);
    contextBridge.exposeInMainWorld('projectApi', ProjectController);
    contextBridge.exposeInMainWorld('platformApi', PlatformController);
};

export default initControllers;