#include "j1Player.h"

#include "Defs.h"
#include "p2Log.h"
#include "j1Input.h"
#include "j1Map.h"
#include "j1Render.h"
#include "j1App.h"
#include "j1EntityFactory.h"
#include "j1Scene.h"
#include "j1Gui.h"

#include "UILabel.h"
#include "UIButton.h"
#include "UIImage.h"

j1Player::j1Player() : j1Module()
{
	name.assign("scene");
}

// Destructor
j1Player::~j1Player() {}

// Called before render is available
bool j1Player::Awake(pugi::xml_node& config)
{
	bool ret = true;

	return ret;
}

// Called before the first frame
bool j1Player::Start()
{
	bool ret = true;

	return ret;
}

bool j1Player::Update(float dt) {

	CheckIfPlaceBuilding();

	if (App->input->GetKey(SDL_SCANCODE_S) == KEY_DOWN)
		if (stables != nullptr) {
			Entity* ent = (Entity*)stables;
			ent->SetDamageLife(20);
			ent->SetStringLife(ent->GetCurrLife(), ent->GetMaxLife());
			if (entityName->GetText() == "Stables")
				HP->SetText(ent->GetStringLife());
		}

	if (App->input->GetKey(SDL_SCANCODE_M) == KEY_DOWN)
		if (mageTower != nullptr) {
			Entity* ent = (Entity*)mageTower;
			ent->SetDamageLife(20);
			ent->SetStringLife(ent->GetCurrLife(), ent->GetMaxLife());
			if (entityName->GetText() == "Magic Tower")
				HP->SetText(ent->GetStringLife());
		}

	if (App->input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
		if (scoutTower != nullptr) {
			Entity* ent = (Entity*)scoutTower;
			ent->SetDamageLife(20);
			ent->SetStringLife(ent->GetCurrLife(), ent->GetMaxLife());
			if (entityName->GetText() == "Scout Tower")
				HP->SetText(ent->GetStringLife());
		}

	if (App->input->GetKey(SDL_SCANCODE_G) == KEY_DOWN)
		if (gryphonAviary != nullptr) {
			Entity* ent = (Entity*)gryphonAviary;
			ent->SetDamageLife(20);
			ent->SetStringLife(ent->GetCurrLife(), ent->GetMaxLife());
			if (entityName->GetText() == "Gryphon Aviary")
				HP->SetText(ent->GetStringLife());
		}

	if (App->input->GetKey(SDL_SCANCODE_C) == KEY_DOWN)
		if (chickenFarm != nullptr) {
			Entity* ent = (Entity*)chickenFarm;
			ent->SetDamageLife(20);
			ent->SetStringLife(ent->GetCurrLife(), ent->GetMaxLife());
			if (entityName->GetText() == "Chicken Farm")
				HP->SetText(ent->GetStringLife());
		}



	return true;
}

iPoint j1Player::GetMousePos() {

	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	return mouseTilePos;
}

void j1Player::CheckIfPlaceBuilding() {

	// Mouse position (world and map coords)
	float auxX = (int)GetMousePos().x;
	float auxY = (int)GetMousePos().y;
	fPoint buildingPos = { auxX, auxY };

	StaticEntityType alphaBuilding = App->scene->GetAlphaBuilding();

	//Creates static entities (buildings)
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) {
		SDL_SetTextureAlphaMod(App->entities->GetHumanBuildingTexture(), 255);

		switch (alphaBuilding) {

		case StaticEntityType_ChickenFarm:
			chickenFarm = App->entities->AddStaticEntity(StaticEntityType_ChickenFarm, buildingPos, App->entities->GetBuildingInfo(StaticEntityType_ChickenFarm), this);
			App->scene->SetAplphaBuilding(StaticEntityType_NoType);
			chickenFarm->SetStringLife(chickenFarm->GetCurrLife(), chickenFarm->GetMaxLife());
			break;

		case StaticEntityType_Stables:
			stables = App->entities->AddStaticEntity(StaticEntityType_Stables, buildingPos, App->entities->GetBuildingInfo(StaticEntityType_Stables), this);
			App->scene->SetAplphaBuilding(StaticEntityType_NoType);
			stables->SetStringLife(stables->GetCurrLife(), stables->GetMaxLife());
			break;

		case StaticEntityType_GryphonAviary:
			gryphonAviary = App->entities->AddStaticEntity(StaticEntityType_GryphonAviary, buildingPos, App->entities->GetBuildingInfo(StaticEntityType_GryphonAviary), this);
			App->scene->SetAplphaBuilding(StaticEntityType_NoType);
			gryphonAviary->SetStringLife(gryphonAviary->GetCurrLife(), gryphonAviary->GetMaxLife());
			break;

		case StaticEntityType_MageTower:
			mageTower = App->entities->AddStaticEntity(StaticEntityType_MageTower, buildingPos, App->entities->GetBuildingInfo(StaticEntityType_MageTower), this);
			App->scene->SetAplphaBuilding(StaticEntityType_NoType);
			mageTower->SetStringLife(mageTower->GetCurrLife(), mageTower->GetMaxLife());
			break;

		case StaticEntityType_ScoutTower:
			scoutTower = App->entities->AddStaticEntity(StaticEntityType_ScoutTower, buildingPos, App->entities->GetBuildingInfo(StaticEntityType_ScoutTower), this);
			App->scene->SetAplphaBuilding(StaticEntityType_NoType);
			scoutTower->SetStringLife(scoutTower->GetCurrLife(), scoutTower->GetMaxLife());
			break;

		case StaticEntityType_NoType:
			break;

		default:
			break;
		}
	}


	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
		App->scene->SetAplphaBuilding(StaticEntityType_NoType);

	//This makes that buildings on the scene aren't printed on alpha
	if (alphaBuilding != StaticEntityType_MaxTypes)
		SDL_SetTextureAlphaMod(App->entities->GetHumanBuildingTexture(), 255);
}

// Called before quitting
bool j1Player::CleanUp()
{
	bool ret = true;

	return ret;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

// Save
bool j1Player::Save(pugi::xml_node& save) const
{
	bool ret = true;

	/*
	if (save.child("gate") == NULL) {
	save.append_child("gate").append_attribute("opened") = gate;
	save.child("gate").append_attribute("fx") = fx;
	}
	else {
	save.child("gate").attribute("opened") = gate;
	save.child("gate").attribute("fx") = fx;
	}
	*/

	return ret;
}

// Load
bool j1Player::Load(pugi::xml_node& save)
{
	bool ret = true;

	/*
	if (save.child("gate") != NULL) {
	gate = save.child("gate").attribute("opened").as_bool();
	fx = save.child("gate").attribute("fx").as_bool();
	}
	*/
	

	return ret;

}


void j1Player::OnStaticEntitiesEvent(StaticEntity* staticEntity, EntitiesEvent entitiesEvent) {

	Entity* ent = (Entity*)staticEntity;

	switch (entitiesEvent)
	{
	case EntitiesEvent_None:
		break;
	case EntitiesEvent_RightClick:
		DeleteEntitiesMenu();
		break;
	case EntitiesEvent_LeftClick:
		DeleteEntitiesMenu();
		if (staticEntity->staticEntityType == StaticEntityType_ChickenFarm)
			MakeEntitiesMenu(ent->GetStringLife(), "Chicken Farm", { 241,34,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_GryphonAviary)
			MakeEntitiesMenu(ent->GetStringLife(), "Gryphon Aviary", { 394,160,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_MageTower)
			MakeEntitiesMenu(ent->GetStringLife(), "Magic Tower", { 394,202,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_ScoutTower)
			MakeEntitiesMenu(ent->GetStringLife(), "Scout Tower", { 394,34,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_Stables)
			MakeEntitiesMenu(ent->GetStringLife(), "Stables", { 241,160,50,41 });
	
		break;
	case EntitiesEvent_Hover:
		break;
	case EntitiesEvent_Leave:
		break;
	case EntitiesEvent_Created:
		DeleteEntitiesMenu();
		if (staticEntity->staticEntityType == StaticEntityType_ChickenFarm)
			MakeEntitiesMenu(ent->GetStringLife(), "Chicken Farm", { 241,34,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_GryphonAviary)
			MakeEntitiesMenu(ent->GetStringLife(), "Gryphon Aviary", { 394,160,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_MageTower)
			MakeEntitiesMenu(ent->GetStringLife(), "Magic Tower", { 394,202,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_ScoutTower)
			MakeEntitiesMenu(ent->GetStringLife(), "Scout Tower", { 394,34,50,41 });

		else if (staticEntity->staticEntityType == StaticEntityType_Stables)
			MakeEntitiesMenu(ent->GetStringLife(), "Stables", { 241,160,50,41 });


		break;
	default:
		break;
	}

}

void j1Player::MakeEntitiesMenu(string HP_text, string entityName_text, SDL_Rect iconDim) {
	

	UILabel_Info labelInfo;
	labelInfo.text = HP_text;
	labelInfo.verticalOrientation = VERTICAL_POS_BOTTOM;
	HP = App->gui->CreateUILabel({ 5, App->scene->entitiesStats->GetLocalRect().h }, labelInfo, nullptr, (UIElement*)App->scene->entitiesStats);

	labelInfo.text = entityName_text;
	labelInfo.verticalOrientation = VERTICAL_POS_TOP;
	entityName = App->gui->CreateUILabel({ 5,5 }, labelInfo, nullptr, (UIElement*)App->scene->entitiesStats);

	UIImage_Info iconInfo;
	iconInfo.texArea = iconDim;
	iconInfo.horizontalOrientation = HORIZONTAL_POS_LEFT;
	iconInfo.verticalOrientation = VERTICAL_POS_CENTER;
	entityIcon = App->gui->CreateUIImage({ 5, App->scene->entitiesStats->GetLocalRect().h/2 }, iconInfo, nullptr, (UIElement*)App->scene->entitiesStats);

}

void j1Player::DeleteEntitiesMenu() {

	App->gui->DestroyElement(HP);
	App->gui->DestroyElement(entityName);
	App->gui->DestroyElement(entityIcon);

}