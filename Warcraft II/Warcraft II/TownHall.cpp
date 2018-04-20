// Upgrades to Keep
// Keep upgrades to Castle

#include "TownHall.h"
#include "j1Player.h"
#include "j1Scene.h"
#include "j1Pathfinding.h"
#include "j1Map.h"

TownHall::TownHall(fPoint pos, iPoint size, int currLife, uint maxLife, const TownHallInfo& townHallInfo, j1Module* listener) :StaticEntity(pos, size, currLife, maxLife, listener), townHallInfo(townHallInfo)
{
	buildingSize = Big;

	iPoint buildingTile = App->map->WorldToMap(pos.x, pos.y);
	App->scene->data[App->scene->w * buildingTile.y + buildingTile.x] = 0u;
	App->scene->data[App->scene->w * buildingTile.y + (buildingTile.x + 1)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 1) + buildingTile.x] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 1) + (buildingTile.x + 1)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y) + (buildingTile.x + 2)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 2) + buildingTile.x] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 2) + (buildingTile.x + 1)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 2) + (buildingTile.x + 2)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 1) + (buildingTile.x + 2)] = 0u;

	App->scene->data[App->scene->w * (buildingTile.y) + (buildingTile.x + 3)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 1) + (buildingTile.x + 3)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 2) + (buildingTile.x + 3)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 3) + (buildingTile.x + 3)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 3) + buildingTile.x] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 3) + (buildingTile.x + 1)] = 0u;
	App->scene->data[App->scene->w * (buildingTile.y + 3) + (buildingTile.x + 2)] = 0u;
	
	texArea = &townHallInfo.townHallCompleteTexArea;
	currLife = maxLife;
	isBuilt = true;
}

void TownHall::Move(float dt)
{

	if (listener != nullptr)
		HandleInput(entityEvent);

	if (App->player->keepUpgrade) {
		if (!startTimer) {
			this->constructionTimer.Start();
			App->player->DeleteEntitiesMenu();
			startTimer = true;
		}
		townHallInfo.townHallType = TownHallType_Castle;
		UpdateAnimations(dt);
	}
	else if (App->player->townHallUpgrade) {
		if (startTimer) {
			this->constructionTimer.Start();
			App->player->DeleteEntitiesMenu();
			startTimer = false;
		}
		townHallInfo.townHallType = TownHallType_Keep;
		UpdateAnimations(dt);
	}
}

// Animations
void TownHall::LoadAnimationsSpeed()
{

}

void TownHall::UpdateAnimations(float dt)
{
	if (constructionTimer.Read() >= constructionTime * 1000) {
		if (townHallInfo.townHallType == TownHallType_Keep) {
			texArea = &townHallInfo.keepCompleteTexArea;
			buildingState = BuildingState_Normal;
			SetMaxLife(1400);
			SetCurrLife(1400);
		}
		else if (townHallInfo.townHallType == TownHallType_Castle) {
			texArea = &townHallInfo.castleCompleteTexArea;
			buildingState = BuildingState_Normal;
			SetMaxLife(1400);
			SetCurrLife(1400);
		}
	}
	else {
		if (townHallInfo.townHallType == TownHallType_Castle) {
			texArea = &townHallInfo.castleInProgressTexArea;
			buildingState = BuildingState_Building;
		}
		else {
			texArea = &townHallInfo.keepInProgressTexArea;
			buildingState = BuildingState_Building;
		}
	}
}