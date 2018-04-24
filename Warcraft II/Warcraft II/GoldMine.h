#ifndef __GoldMine_H__
#define __GoldMine_H__

#include "StaticEntity.h"

struct GoldMineInfo
{
	SDL_Rect completeTexArea = { 0,0,0,0 };
	SDL_Rect inProgressTexArea = { 0,0,0,0 };

	int maxLife = 0;
	iPoint size{ 0,0 };
	uint life = 0u;
	float speed = 0.0f;
};

enum GoldMineState {
	
	GoldMine_Untouched,
	GoldMine_Gathering,
	GoldMine_Gathered,

};

class GoldMine :public StaticEntity
{
public:

	GoldMine(fPoint pos, iPoint size, int currLife, uint maxLife, const GoldMineInfo& goldMineInfo, j1Module* listener);
	~GoldMine() {};

	void Move(float dt);

	// Gather gold
	bool IsUnitGatheringGold() const;
	void SetUnitGatheringGold(bool isUnitGatheringGold);

	// Tex area
	void SwapTexArea();


public:

	GoldMineState goldMineState = GoldMine_Untouched;

private:

	GoldMineInfo goldMineInfo;
	EntitiesEvent entityEvent = EntitiesEvent_NONE;

	bool startTimer = true;

	// Gather gold
	bool isUnitGatheringGold = false;
};

#endif //__GoldMine_H__