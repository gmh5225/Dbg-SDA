import { invokerFactory } from '../utils';
import { ProjectController } from '../../api/project';
import { ObjectId, ObjectChangeType } from '../../api/common';

const invoke = invokerFactory("Project");

const ProjectControllerImpl: ProjectController = {
    getRecentProjects: () =>
        invoke("getRecentProjects"),
    
    updateRecentProjectsWithPath: (path: string, changeType: ObjectChangeType) =>
        invoke("updateRecentProjectsWithPath", path, changeType),
    
    getActiveProjects: () =>
        invoke("getActiveProjects"),

    getActiveProject: (id: ObjectId) =>
        invoke("getActiveProject", id),

    openProject: (path: string) =>
        invoke("openProject", path),

    createProject: (path: string, platformName: string) =>
        invoke("createProject", path, platformName),

    deleteProject: (path: string) =>
        invoke("deleteProject", path),
};

export default ProjectControllerImpl;