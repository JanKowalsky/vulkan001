#ifndef SCENE_H
#define SCENE_H

class Scene
{
public:
	virtual void render() = 0;
	virtual void onResize() = 0;
	virtual InputManager& getInputManager() = 0;
	virtual Timer& getTimer() = 0;
};

#endif //SCENE_H