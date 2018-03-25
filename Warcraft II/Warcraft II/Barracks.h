#ifndef __Barracks_H__
#define __Barracks_H__

#include "StaticEntity.h"

enum BarracksType 
{
	BarracksType_Barracks,
	BarracksType_Barracks2
};

struct BarracksInfo
{
	BarracksType barracksType = BarracksType_Barracks;
	SDL_Rect barracksCompleteTexArea = { 0,0,0,0 };
	SDL_Rect barracks2CompleteTexArea = { 0,0,0,0 };
	SDL_Rect constructionPlanks1 = { 0,0,0,0 };
	SDL_Rect constructionPlanks2 = { 0,0,0,0 };
	int barracks1MaxLife = 0;
	int barracks2MaxLife = 0;
	iPoint size{ 0,0 };
	uint life = 0u;
	float speed = 0.0f;
};

class Barracks :public StaticEntity
{
public:

	Barracks(fPoint pos, iPoint size, int maxLife, const BarracksInfo& barracksInfo, j1Module* listener);
	~Barracks() {};

	void Move(float dt);

	// Animations
	void LoadAnimationsSpeed();
	void UpdateAnimations(float dt);

private:

	BarracksInfo barracksInfo;

	EntitiesEvent EntityEvent = EntitiesEvent_CREATED;
};

#endif //__Barracks_H__