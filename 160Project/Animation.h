#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Mesh.h"

#ifndef PI
#define PI  3.14159265
#endif


class Animation
{
public:
	enum AnimationType {ROTATE, SCALE, TRANSLATE, POSITION, TRANSPARENCY};
	enum EasingType {LINEAR, ELASTIC_OUT, QUAD_OUT, SINE_OUT, NONE};
	Animation(AnimationType type);

	//Pass in the initial values, read from somewhere.
	//Typically 0,0,0 if object is untranslated.
	//1.0, 1.0f, 1.0f if object is unscaled.
	//0, 0, 0 if object is unrotated.
	//Pass in current center if moving to an absolute position.
	void setStart(Mesh* mesh, glm::vec3& start);
	//Enter final scale, rotation (degrees), or position
	void setGoal(const glm::vec3& goal, float duration, EasingType easing = LINEAR);
	//Calls the appropriate transformation function in the mesh.
	void update(int time);

	void chain(Animation& animation);

	bool hasEnded() {return ended;}
	bool containsLink() {return hasLink;}
	Animation* getLink(){ return nextLink;}

	~Animation();

private:
	glm::vec3 calculateStep(float t);
	AnimationType animationType;
	EasingType easingType;
	glm::vec3 start;
	glm::vec3 changeInValue;
	glm::vec3 goal;
	glm::vec3 currentTransformation; // a portion of the goal trans
	int startTime;
	int timeLeft;
	float duration;
	bool started;
	bool hasLink;
	bool ended;
	Mesh* mesh;
	Animation* nextLink;
};
