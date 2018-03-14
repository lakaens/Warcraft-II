#include "Defs.h"
#include "p2Log.h"

#include "j1Module.h"
#include "j1App.h"

#include "j1EntityFactory.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Scene.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1Collision.h"
#include "j1Input.h"

#include "Entity.h"
#include "DynamicEntity.h"
#include "StaticEntity.h"

j1EntityFactory::j1EntityFactory()
{
	name.assign("entities");
}

// Destructor
j1EntityFactory::~j1EntityFactory()
{
}

bool j1EntityFactory::Awake(pugi::xml_node& config) {

	bool ret = true;

	// Spritesheets
	pugi::xml_node spritesheets = config.child("spritesheets");
	humanBuildingsTexName = spritesheets.child("humanBuildings").attribute("name").as_string();

	// Static entities
	pugi::xml_node staticEntities = config.child("staticEntities");

	pugi::xml_node aux = staticEntities.child("townHall").child("sprites");
	townHallInfo.townHallCompleteTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	townHallInfo.townHallInProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("keep").child("sprites");
	townHallInfo.keepCompleteTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	townHallInfo.keepInProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("castle").child("sprites");
	townHallInfo.castleCompleteTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	townHallInfo.castleInProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("chickenFarm").child("sprites");
	chickenFarmInfo.completeTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	chickenFarmInfo.inProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("scoutTower").child("sprites");
	scoutTowerInfo.completeTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	scoutTowerInfo.inProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("barracks").child("sprites");
	barracksInfo.barracksCompleteTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };

	aux = staticEntities.child("barracks2").child("sprites");
	barracksInfo.barracks2CompleteTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };

	aux = staticEntities.child("mageTower").child("sprites");
	mageTowerInfo.completeTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	mageTowerInfo.inProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("gryphonAviary").child("sprites");
	gryphonAviaryInfo.completeTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	gryphonAviaryInfo.inProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	aux = staticEntities.child("stables").child("sprites");
	stablesInfo.completeTexArea = { aux.child("complete").attribute("x").as_int(), aux.child("complete").attribute("y").as_int(), aux.child("complete").attribute("w").as_int(), aux.child("complete").attribute("h").as_int() };
	stablesInfo.inProgressTexArea = { aux.child("inProgress").attribute("x").as_int(), aux.child("inProgress").attribute("y").as_int(), aux.child("inProgress").attribute("w").as_int(), aux.child("inProgress").attribute("h").as_int() };

	return ret;
}

bool j1EntityFactory::Start()
{
	bool ret = true;

	LOG("Loading entities textures");

	humanBuildingsTex = App->tex->Load(humanBuildingsTexName.data());

	return ret;
}

bool j1EntityFactory::PreUpdate()
{
	bool ret = true;

	// Spawn entities
	list<Entity*>::const_iterator it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {

		fPoint pos = (*it)->GetPosition();
		int x = pos.x * App->scene->scale;
		int y = pos.y * App->scene->scale;
		App->map->WorldToMap(x, y);

		// Move the entity from the waiting list to the active list
		if ((*it)->entityType == EntityType_DynamicEntity) {

			activeDynamicEntities.push_back((DynamicEntity*)(*it));
			LOG("Spawning dynamic entity at tile %d,%d", x, y);
		}
		else if ((*it)->entityType == EntityType_StaticEntity) {

			activeStaticEntities.push_back((StaticEntity*)(*it));
			LOG("Spawning static entity at tile %d,%d", x, y);
		}

		it++;
	}
	toSpawnEntities.clear();

	return ret;
}

// Called before render is available
bool j1EntityFactory::Update(float dt)
{
	bool ret = true;

	// Update active dynamic entities
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {
		(*it)->Move(dt);
		it++;
	}

	// Mouse position (world and map coords)
	// TODO: Valdivia. Aix� no s'ha de fer aqu�. Est� molt cutre a sobre xd. Porta-ho al m�dul de player o aix� per fer-ho ben fet
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {
		SDL_SetTextureAlphaMod(humanBuildingsTex, 100);
		DrawStaticEntityPreview(StaticEntityType_TownHall, { mouseTilePos.x, mouseTilePos.y });
		SDL_SetTextureAlphaMod(humanBuildingsTex, 255);
	}
	// TODO: Valdivia. Fins aqu�

	return ret;
}

void j1EntityFactory::Draw()
{
	// Blit active entities

	// Dynamic entities (one texture per dynamic entity)
	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {

		switch ((*dynEnt)->dynamicEntityType) {

			// Player
		case DynamicEntityType_Footman:
			(*dynEnt)->Draw(footmanTex);
			break;
		case DynamicEntityType_ElvenArcher:
			(*dynEnt)->Draw(elvenArcherTex);
			break;
		case DynamicEntityType_GryphonRider:
			(*dynEnt)->Draw(gryphonRiderTex);
			break;
		case DynamicEntityType_Mage:
			(*dynEnt)->Draw(mageTex);
			break;
		case DynamicEntityType_Paladin:
			(*dynEnt)->Draw(paladinTex);
			break;

		case DynamicEntityType_Turalyon:
			(*dynEnt)->Draw(turalyonTex);
			break;
		case DynamicEntityType_Khadgar:
			(*dynEnt)->Draw(khadgarTex);
			break;
		case DynamicEntityType_Alleria:
			(*dynEnt)->Draw(alleriaTex);
			break;

			// Enemy
		case DynamicEntityType_Grunt:
			(*dynEnt)->Draw(gruntTex);
			break;
		case DynamicEntityType_TrollAxethrower:
			(*dynEnt)->Draw(trollAxethrowerTex);
			break;
		case DynamicEntityType_Dragon:
			(*dynEnt)->Draw(dragonTex);
			break;

		default:
			break;
		}

		dynEnt++;
	}

	// Static entities (altas textures for every type of static entity)
	list<StaticEntity*>::const_iterator statEnt = activeStaticEntities.begin();

	while (statEnt != activeStaticEntities.end()) {

		switch ((*statEnt)->staticEntityCategory) {

		case StaticEntityCategory_HumanBuilding:
			(*statEnt)->Draw(humanBuildingsTex);
			break;
		case StaticEntityCategory_NeutralBuilding:
			(*statEnt)->Draw(neutralBuildingsTex);
			break;
		case StaticEntityCategory_OrcishBuilding:
			(*statEnt)->Draw(orcishBuildingsTex);
			break;

		default:
			break;
		}

		statEnt++;
	}
}

void j1EntityFactory::DrawStaticEntityPreview(StaticEntityType staticEntityType, iPoint mousePos)
{
	switch (staticEntityType) {

	case StaticEntityType_TownHall:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &townHallInfo.townHallCompleteTexArea);
		break;
	case StaticEntityType_ChickenFarm:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &chickenFarmInfo.completeTexArea);
		break;
	case StaticEntityType_Barracks:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &barracksInfo.barracksCompleteTexArea);
		break;
	case StaticEntityType_MageTower:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &mageTowerInfo.completeTexArea);
		break;
	case StaticEntityType_GryphonAviary:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &gryphonAviaryInfo.completeTexArea);
		break;
	case StaticEntityType_Stables:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &stablesInfo.completeTexArea);
		break;
	case StaticEntityType_ScoutTower:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &scoutTowerInfo.completeTexArea);
		break;
	case StaticEntityType_PlayerGuardTower:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &playerGuardTowerInfo.completeTexArea);
		break;
	case StaticEntityType_PlayerCannonTower:
		App->render->Blit(humanBuildingsTex, mousePos.x, mousePos.y, &playerCannonTowerInfo.completeTexArea);
		break;

	default:
		break;
	}
}


bool j1EntityFactory::PostUpdate()
{
	bool ret = true;

	// Remove active entities
	/// Remove dynamic entities
	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {
		if ((*dynEnt)->remove) {
			delete *dynEnt;
			activeDynamicEntities.remove(*dynEnt);
		}

		dynEnt++;
	}

	/// Remove static entities
	list<StaticEntity*>::const_iterator statEnt = activeStaticEntities.begin();

	while (statEnt != activeStaticEntities.end()) {
		if ((*statEnt)->remove) {
			delete *statEnt;
			activeStaticEntities.remove(*statEnt);
		}

		statEnt++;
	}

	return ret;
}

// Called before quitting
bool j1EntityFactory::CleanUp()
{
	bool ret = true;

	LOG("Freeing all entities");

	// Remove active entities
	/// Remove dynamic entities
	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {
		if ((*dynEnt)->remove) {
			delete *dynEnt;
			activeDynamicEntities.remove(*dynEnt);
		}

		dynEnt++;
	}
	activeDynamicEntities.clear();

	/// Remove static entities
	list<StaticEntity*>::const_iterator statEnt = activeStaticEntities.begin();

	while (statEnt != activeStaticEntities.end()) {
		if ((*statEnt)->remove) {
			delete *statEnt;
			activeStaticEntities.remove(*statEnt);
		}

		statEnt++;
	}
	activeStaticEntities.clear();

	// Remove to spawn entities
	list<Entity*>::const_iterator it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {
		delete *it;
		it++;
	}
	toSpawnEntities.clear();

	// Free all textures
	App->tex->UnLoad(footmanTex);

	return ret;
}

StaticEntity* j1EntityFactory::AddStaticEntity(StaticEntityType staticEntityType, fPoint pos, iPoint size, uint life, const EntityInfo& entityInfo)
{
	switch (staticEntityType) {
	
	case StaticEntityType_TownHall:
	{
		TownHall* townHall = new TownHall(pos, size, life, (const TownHallInfo&)entityInfo);
		townHall->entityType = EntityType_StaticEntity;
		townHall->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		townHall->staticEntityType = StaticEntityType_TownHall;

		toSpawnEntities.push_back((Entity*)townHall);
		return (StaticEntity*)townHall;
	}
		break;

	case StaticEntityType_ChickenFarm:
	{
		ChickenFarm* chickenFarm = new ChickenFarm(pos, size, life, (const ChickenFarmInfo&)entityInfo);
		chickenFarm->entityType = EntityType_StaticEntity;
		chickenFarm->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		chickenFarm->staticEntityType = StaticEntityType_ChickenFarm;

		toSpawnEntities.push_back((Entity*)chickenFarm);
		return (StaticEntity*)chickenFarm;
	}
		break;

	case StaticEntityType_Barracks:
	{
		Barracks* barracks = new Barracks(pos, size, life, (const BarracksInfo&)entityInfo);
		barracks->entityType = EntityType_StaticEntity;
		barracks->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		barracks->staticEntityType = StaticEntityType_Barracks;

		toSpawnEntities.push_back((Entity*)barracks);
		return (StaticEntity*)barracks;
	}
		break;

	case StaticEntityType_MageTower:
	{
		MageTower* mageTower = new MageTower(pos, size, life, (const MageTowerInfo&)entityInfo);
		mageTower->entityType = EntityType_StaticEntity;
		mageTower->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		mageTower->staticEntityType = StaticEntityType_MageTower;

		toSpawnEntities.push_back((Entity*)mageTower);
		return (StaticEntity*)mageTower;
	}
		break;

	case StaticEntityType_GryphonAviary:
	{
		GryphonAviary* gryphonAviary = new GryphonAviary(pos, size, life, (const GryphonAviaryInfo&)entityInfo);
		gryphonAviary->entityType = EntityType_StaticEntity;
		gryphonAviary->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		gryphonAviary->staticEntityType = StaticEntityType_GryphonAviary;

		toSpawnEntities.push_back((Entity*)gryphonAviary);
		return (StaticEntity*)gryphonAviary;
	}
		break;

	case StaticEntityType_Stables:
	{
		Stables* stables = new Stables(pos, size, life, (const StablesInfo&)entityInfo);
		stables->entityType = EntityType_StaticEntity;
		stables->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		stables->staticEntityType = StaticEntityType_Stables;

		toSpawnEntities.push_back((Entity*)stables);
		return (StaticEntity*)stables;
	}
		break;

	case StaticEntityType_ScoutTower:
	{
		ScoutTower* scoutTower = new ScoutTower(pos, size, life, (const ScoutTowerInfo&)entityInfo);
		scoutTower->entityType = EntityType_StaticEntity;
		scoutTower->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		scoutTower->staticEntityType = StaticEntityType_ScoutTower;

		toSpawnEntities.push_back((Entity*)scoutTower);
		return (StaticEntity*)scoutTower;
	}
		break;

	case StaticEntityType_PlayerGuardTower:
	{
		PlayerGuardTower* playerGuardTower = new PlayerGuardTower(pos, size, life, (const PlayerGuardTowerInfo&)entityInfo);
		playerGuardTower->entityType = EntityType_StaticEntity;
		playerGuardTower->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		playerGuardTower->staticEntityType = StaticEntityType_PlayerGuardTower;

		toSpawnEntities.push_back((Entity*)playerGuardTower);
		return (StaticEntity*)playerGuardTower;
	}
		break;

	case StaticEntityType_PlayerCannonTower:
	{
		PlayerCannonTower* playerCannonTower = new PlayerCannonTower(pos, size, life, (const PlayerCannonTowerInfo&)entityInfo);
		playerCannonTower->entityType = EntityType_StaticEntity;
		playerCannonTower->staticEntityCategory = StaticEntityCategory_HumanBuilding;
		playerCannonTower->staticEntityType = StaticEntityType_PlayerCannonTower;

		toSpawnEntities.push_back((Entity*)playerCannonTower);
		return (StaticEntity*)playerCannonTower;
	}
		break;

	case StaticEntityType_GoldMine:
	{
		GoldMine* goldMine = new GoldMine(pos, size, life, (const GoldMineInfo&)entityInfo);
		goldMine->entityType = EntityType_StaticEntity;
		goldMine->staticEntityCategory = StaticEntityCategory_NeutralBuilding;
		goldMine->staticEntityType = StaticEntityType_GoldMine;

		toSpawnEntities.push_back((Entity*)goldMine);
		return (StaticEntity*)goldMine;
	}
		break;

	case StaticEntityType_Runestone:
	{
		Runestone* runestone = new Runestone(pos, size, life, (const RunestoneInfo&)entityInfo);
		runestone->entityType = EntityType_StaticEntity;
		runestone->staticEntityCategory = StaticEntityCategory_NeutralBuilding;
		runestone->staticEntityType = StaticEntityType_Runestone;

		toSpawnEntities.push_back((Entity*)runestone);
		return (StaticEntity*)runestone;
	}
		break;

	case StaticEntityType_WatchTower:
	{
		WatchTower* watchTower = new WatchTower(pos, size, life, (const WatchTowerInfo&)entityInfo);
		watchTower->entityType = EntityType_StaticEntity;
		watchTower->staticEntityCategory = StaticEntityCategory_OrcishBuilding;
		watchTower->staticEntityType = StaticEntityType_WatchTower;

		toSpawnEntities.push_back((Entity*)watchTower);
		return (StaticEntity*)watchTower;
	}
		break;

	case StaticEntityType_EnemyGuardTower:
	{
		EnemyGuardTower* enemyGuardTower = new EnemyGuardTower(pos, size, life, (const EnemyGuardTowerInfo&)entityInfo);
		enemyGuardTower->entityType = EntityType_StaticEntity;
		enemyGuardTower->staticEntityCategory = StaticEntityCategory_OrcishBuilding;
		enemyGuardTower->staticEntityType = StaticEntityType_EnemyGuardTower;

		toSpawnEntities.push_back((Entity*)enemyGuardTower);
		return (StaticEntity*)enemyGuardTower;
	}
		break;

	case StaticEntityType_EnemyCannonTower:
	{
		EnemyCannonTower* enemyCannonTower = new EnemyCannonTower(pos, size, life, (const EnemyCannonTowerInfo&)entityInfo);
		enemyCannonTower->entityType = EntityType_StaticEntity;
		enemyCannonTower->staticEntityCategory = StaticEntityCategory_OrcishBuilding;
		enemyCannonTower->staticEntityType = StaticEntityType_EnemyCannonTower;

		toSpawnEntities.push_back((Entity*)enemyCannonTower);
		return (StaticEntity*)enemyCannonTower;
	}
		break;

	default:

		return nullptr;
	}
}

DynamicEntity* j1EntityFactory::AddDynamicEntity(DynamicEntityType dynamicEntityType, fPoint pos, iPoint size, uint life, float speed, const EntityInfo& entityInfo)
{
	switch (dynamicEntityType) {

	case DynamicEntityType_Footman:
	{
		Footman* footman = new Footman(pos, size, life, speed, (const FootmanInfo&)entityInfo);
		footman->entityType = EntityType_DynamicEntity;
		footman->dynamicEntityType = DynamicEntityType_Footman;

		toSpawnEntities.push_back((Entity*)footman);
		return (DynamicEntity*)footman;
	}
		break;

	case DynamicEntityType_ElvenArcher:
	{
		ElvenArcher* elvenArcher = new ElvenArcher(pos, size, life, speed, (const ElvenArcherInfo&)entityInfo);
		elvenArcher->entityType = EntityType_DynamicEntity;
		elvenArcher->dynamicEntityType = DynamicEntityType_ElvenArcher;

		toSpawnEntities.push_back((Entity*)elvenArcher);
		return (DynamicEntity*)elvenArcher;
	}
		break;

	case DynamicEntityType_GryphonRider:
	{
		GryphonRider* gryphonRider = new GryphonRider(pos, size, life, speed, (const GryphonRiderInfo&)entityInfo);
		gryphonRider->entityType = EntityType_DynamicEntity;
		gryphonRider->dynamicEntityType = DynamicEntityType_GryphonRider;

		toSpawnEntities.push_back((Entity*)gryphonRider);
		return (DynamicEntity*)gryphonRider;
	}
		break;

	case DynamicEntityType_Mage:
	{
		Mage* mage = new Mage(pos, size, life, speed, (const MageInfo&)entityInfo);
		mage->entityType = EntityType_DynamicEntity;
		mage->dynamicEntityType = DynamicEntityType_Mage;

		toSpawnEntities.push_back((Entity*)mage);
		return (DynamicEntity*)mage;
	}
		break;

	case DynamicEntityType_Paladin:
	{
		Paladin* paladin = new Paladin(pos, size, life, speed, (const PaladinInfo&)entityInfo);
		paladin->entityType = EntityType_DynamicEntity;
		paladin->dynamicEntityType = DynamicEntityType_Paladin;

		toSpawnEntities.push_back((Entity*)paladin);
		return (DynamicEntity*)paladin;
	}
		break;

	case DynamicEntityType_Turalyon:
	{
		Turalyon* turalyon = new Turalyon(pos, size, life, speed, (const TuralyonInfo&)entityInfo);
		turalyon->entityType = EntityType_DynamicEntity;
		turalyon->dynamicEntityType = DynamicEntityType_Turalyon;

		toSpawnEntities.push_back((Entity*)turalyon);
		return (DynamicEntity*)turalyon;
	}
		break;

	case DynamicEntityType_Khadgar:
	{
		Khadgar* khadgar = new Khadgar(pos, size, life, speed, (const KhadgarInfo&)entityInfo);
		khadgar->entityType = EntityType_DynamicEntity;
		khadgar->dynamicEntityType = DynamicEntityType_Khadgar;

		toSpawnEntities.push_back((Entity*)khadgar);
		return (DynamicEntity*)khadgar;
	}
		break;

	case DynamicEntityType_Alleria:
	{
		Alleria* alleria = new Alleria(pos, size, life, speed, (const AlleriaInfo&)entityInfo);
		alleria->entityType = EntityType_DynamicEntity;
		alleria->dynamicEntityType = DynamicEntityType_Alleria;

		toSpawnEntities.push_back((Entity*)alleria);
		return (DynamicEntity*)alleria;
	}
		break;

	case DynamicEntityType_Grunt:
	{
		Grunt* grunt = new Grunt(pos, size, life, speed, (const GruntInfo&)entityInfo);
		grunt->entityType = EntityType_DynamicEntity;
		grunt->dynamicEntityType = DynamicEntityType_Grunt;

		toSpawnEntities.push_back((Entity*)grunt);
		return (DynamicEntity*)grunt;
	}
		break;

	case DynamicEntityType_TrollAxethrower:
	{
		TrollAxethrower* trollAxethrower = new TrollAxethrower(pos, size, life, speed, (const TrollAxethrowerInfo&)entityInfo);
		trollAxethrower->entityType = EntityType_DynamicEntity;
		trollAxethrower->dynamicEntityType = DynamicEntityType_TrollAxethrower;

		toSpawnEntities.push_back((Entity*)trollAxethrower);
		return (DynamicEntity*)trollAxethrower;
	}
		break;

	case DynamicEntityType_Dragon:
	{
		Dragon* dragon = new Dragon(pos, size, life, speed, (const DragonInfo&)entityInfo);
		dragon->entityType = EntityType_DynamicEntity;
		dragon->dynamicEntityType = DynamicEntityType_Dragon;

		toSpawnEntities.push_back((Entity*)dragon);
		return (DynamicEntity*)dragon;
	}
		break;

	default:

		return nullptr;
	}
}

// -------------------------------------------------------------
// -------------------------------------------------------------

bool j1EntityFactory::Load(pugi::xml_node& save)
{
	bool ret = true;

	pugi::xml_node node;

	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {
		// MYTODO: Add some code here
		dynEnt++;
	}

	list<StaticEntity*>::const_iterator statEnt = activeStaticEntities.begin();

	while (statEnt != activeStaticEntities.end()) {
		// MYTODO: Add some code here
		statEnt++;
	}

	return ret;
}

bool j1EntityFactory::Save(pugi::xml_node& save) const
{
	bool ret = true;

	pugi::xml_node node;

	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {
		// MYTODO: Add some code here
		dynEnt++;
	}

	list<StaticEntity*>::const_iterator statEnt = activeStaticEntities.begin();

	while (statEnt != activeStaticEntities.end()) {
		// MYTODO: Add some code here
		statEnt++;
	}

	return ret;
}