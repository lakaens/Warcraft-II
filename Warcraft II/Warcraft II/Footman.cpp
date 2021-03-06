#include "Defs.h"
#include "p2Log.h"

#include "Footman.h"

#include "j1App.h"
#include "j1Render.h"
#include "j1Collision.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Movement.h"
#include "j1PathManager.h"
#include "Goal.h"
#include "j1Audio.h"
#include "j1Printer.h"

#include "UILifeBar.h"

#include "j1Scene.h" // isFrameByFrame
#include "j1Input.h" // isFrameByFrame

#include "Brofiler\Brofiler.h"

Footman::Footman(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const FootmanInfo& footmanInfo, j1Module* listener) :DynamicEntity(pos, size, currLife, maxLife, unitInfo, listener), footmanInfo(footmanInfo)
{
	*(ENTITY_CATEGORY*)&entityType = EntityCategory_DYNAMIC_ENTITY;
	*(ENTITY_TYPE*)&dynamicEntityType = EntityType_FOOTMAN;
	*(EntitySide*)&entitySide = EntitySide_Player;

	// XML loading
	/// Animations
	FootmanInfo info = (FootmanInfo&)App->entities->GetUnitInfo(EntityType_FOOTMAN);
	this->unitInfo = this->footmanInfo.unitInfo;
	this->footmanInfo.up = info.up;
	this->footmanInfo.down = info.down;
	this->footmanInfo.left = info.left;
	this->footmanInfo.right = info.right;
	this->footmanInfo.upLeft = info.upLeft;
	this->footmanInfo.upRight = info.upRight;
	this->footmanInfo.downLeft = info.downLeft;
	this->footmanInfo.downRight = info.downRight;

	this->footmanInfo.attackUp = info.attackUp;
	this->footmanInfo.attackDown = info.attackDown;
	this->footmanInfo.attackLeft = info.attackLeft;
	this->footmanInfo.attackRight = info.attackRight;
	this->footmanInfo.attackUpLeft = info.attackUpLeft;
	this->footmanInfo.attackUpRight = info.attackUpRight;
	this->footmanInfo.attackDownLeft = info.attackDownLeft;
	this->footmanInfo.attackDownRight = info.attackDownRight;

	this->footmanInfo.deathUp = info.deathUp;
	this->footmanInfo.deathDown = info.deathDown;

	this->size = this->unitInfo.size;
	offsetSize = this->unitInfo.offsetSize;

	LoadAnimationsSpeed();

	// Set the color of the entity
	color = ColorDarkBlue;
	colorName = "BlueFootman";

	// Initialize the goals
	brain->RemoveAllSubgoals();

	// LifeBar creation
	UILifeBar_Info lifeBarInfo;
	lifeBarInfo.background = { 241,336,45,8 };
	lifeBarInfo.bar = { 241, 369,44,8 };
	lifeBarInfo.maxLife = this->maxLife;
	lifeBarInfo.life = this->currLife;
	lifeBarInfo.maxWidth = lifeBarInfo.bar.w;

	lifeBar = App->gui->CreateUILifeBar({ (int)pos.x - lifeBarMarginX, (int)pos.y - lifeBarMarginY }, lifeBarInfo, (j1Module*)this, nullptr, true);
	lifeBar->SetPriorityDraw(PriorityDraw_LIFEBAR_INGAME);

	// Collisions
	CreateEntityCollider(EntitySide_Player);
	sightRadiusCollider = CreateRhombusCollider(ColliderType_PlayerSightRadius, this->unitInfo.sightRadius, DistanceHeuristic_DistanceManhattan);
	attackRadiusCollider = CreateRhombusCollider(ColliderType_PlayerAttackRadius, this->unitInfo.attackRadius, DistanceHeuristic_DistanceTo);
	entityCollider->isTrigger = true;
	sightRadiusCollider->isTrigger = true;
	attackRadiusCollider->isTrigger = true;
}

void Footman::Move(float dt)
{
	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// ---------------------------------------------------------------------

	//LOG("Goals: %i", brain->GetSubgoalsList().size());
	//LOG("Targets: %i", targets.size());

	// Is the unit dead?
	/// The unit must fit the tile (it is more attractive for the player)
	if (singleUnit != nullptr) {

		if (currLife <= 0
			&& unitState != UnitState_Die
			&& singleUnit->IsFittingTile()
			&& !isDead) {

			isBlitSavedGroupSelection = false;
			isBlitSelectedGroupSelection = false;

			App->audio->PlayFx(App->audio->GetFX().humanDeath, 0);

			isDead = true;

			// Remove the entity from the unitsSelected list
			App->entities->RemoveUnitFromUnitsSelected(this);

			brain->RemoveAllSubgoals();

			unitState = UnitState_NoState;

			// Remove Movement (so other units can walk above them)
			App->entities->InvalidateMovementEntity(this);

			// Remove any path request
			pathPlanner->SetSearchRequested(false);
			pathPlanner->SetSearchCompleted(false);
			App->pathmanager->UnRegister(pathPlanner);

			if (singleUnit != nullptr)
				delete singleUnit;
			singleUnit = nullptr;

			// Invalidate colliders
			sightRadiusCollider->isValid = false;
			attackRadiusCollider->isValid = false;
			entityCollider->isValid = false;

			LOG("A Footman has died");
		}
	}

	// Update currTarget
	if (currTarget != nullptr) {

		if (currTarget->isRemoveNeeded || currTarget->target->isRemove)
			currTarget = nullptr;
	}

	if (!isDead && isValid) {

		// PROCESS THE COMMANDS

		// 1. Remove attack
		bool isAttacking = false;

		switch (unitCommand) {

		case UnitCommand_Stop:
		case UnitCommand_MoveToPosition:
		case UnitCommand_Patrol:
		case UnitCommand_GatherGold:
		case UnitCommand_HealRunestone:
		case UnitCommand_RescuePrisoner:

			/// The unit could be attacking before this command
			if (currTarget != nullptr) {

				isAttacking = true;
				currTarget->target->RemoveAttackingUnit(this);
				currTarget = nullptr;
			}

			isHitting = false;
			///

			break;

		case UnitCommand_NoCommand:
		default:

			break;
		}

		// 2. Actual command
		switch (unitCommand) {

		case UnitCommand_Stop:

			if (singleUnit->IsFittingTile()) {

				brain->RemoveAllSubgoals();

				unitState = UnitState_Idle;
				unitCommand = UnitCommand_NoCommand;
			}

			break;

		case UnitCommand_MoveToPosition:

			// The goal of the unit has been changed manually
			if (singleUnit->isGoalChanged) {

				if (singleUnit->IsFittingTile()) {

					if (unitState == UnitState_AttackTarget || ((unitState == UnitState_Idle || unitState == UnitState_Walk) && isAttacking))
						isRunAway = true;

					brain->RemoveAllSubgoals();
					brain->AddGoal_MoveToPosition(singleUnit->goal);

					unitState = UnitState_Walk;
					unitCommand = UnitCommand_NoCommand;
				}
			}

			break;

		case UnitCommand_Patrol:

			// The goal of the unit has been changed manually (to patrol)
			if (singleUnit->isGoalChanged) {

				if (singleUnit->IsFittingTile()) {

					brain->RemoveAllSubgoals();
					brain->AddGoal_Patrol(singleUnit->currTile, singleUnit->goal);

					unitState = UnitState_Patrol;
					unitCommand = UnitCommand_NoCommand;
				}
			}

			break;

		case UnitCommand_AttackTarget:

			if (newTarget != nullptr) {

				if (singleUnit->IsFittingTile()) {

					brain->RemoveAllSubgoals();
					brain->AddGoal_AttackTarget(newTarget);

					newTarget = nullptr;

					unitState = UnitState_AttackTarget;
					unitCommand = UnitCommand_NoCommand;
				}
			}

			break;

		case UnitCommand_GatherGold:

			if (goldMine != nullptr) {

				if (goldMine->buildingState == BuildingState_Normal) {

					if (singleUnit->IsFittingTile()) {

						brain->RemoveAllSubgoals();
						brain->AddGoal_GatherGold(goldMine);

						unitState = UnitState_GatherGold;
						unitCommand = UnitCommand_NoCommand;
					}
				}
			}

			break;

		case UnitCommand_HealRunestone:

			if (runestone != nullptr) {

				if (runestone->buildingState == BuildingState_Normal) {

					if (singleUnit->IsFittingTile()) {

						brain->RemoveAllSubgoals();
						brain->AddGoal_HealRunestone(runestone);

						unitState = UnitState_HealRunestone;
						unitCommand = UnitCommand_NoCommand;
					}
				}
			}

			break;

		case UnitCommand_RescuePrisoner:

			if (prisoner != nullptr) {

				if (singleUnit->IsFittingTile()) {

					brain->RemoveAllSubgoals();
					brain->AddGoal_RescuePrisoner(prisoner);

					unitState = UnitState_RescuePrisoner;
					unitCommand = UnitCommand_NoCommand;
				}
			}

			break;

		case UnitCommand_NoCommand:
		default:

			break;
		}
	}

	if (!isDead) {

		// PROCESS THE CURRENTLY ACTIVE GOAL
		brain->Process(dt);

		// Update targets to be removed
		UpdateTargetsToRemove();
	}

	UnitStateMachine(dt);
	HandleInput(entityEvent);

	// Update animations
	if (!isStill || isHitting)
		UpdateAnimationsSpeed(dt);

	ChangeAnimation();

	if (!isDead && lastColliderUpdateTile != singleUnit->currTile) {

		// Update colliders
		UpdateEntityColliderPos();
		UpdateRhombusColliderPos(sightRadiusCollider, unitInfo.sightRadius, DistanceHeuristic_DistanceManhattan);
		UpdateRhombusColliderPos(attackRadiusCollider, unitInfo.attackRadius, DistanceHeuristic_DistanceTo);

		lastColliderUpdateTile = singleUnit->currTile;
	}

	// Update unit's life bar
	if (lifeBar != nullptr) {

		lifeBar->SetLocalPos({ (int)pos.x - lifeBarMarginX, (int)pos.y - lifeBarMarginY });

		if (currLife >= 0)
			lifeBar->SetLife(currLife);
	}

	// Blit group selection
	if (isBlitSelectedGroupSelection) {

		float alphaSpeed = 250.0f;
		alphaSelectedGroupSelection -= alphaSpeed * dt;

		if (alphaSelectedGroupSelection <= 0) {
			alphaSelectedGroupSelection = 0;
			isBlitSelectedGroupSelection = false;
		}

		SDL_Color alphaColor = color;
		alphaColor.a = alphaSelectedGroupSelection;

		const SDL_Rect entitySizeA = { pos.x + offsetSize.x - 1, pos.y + offsetSize.y - 1, size.x + 2, size.y + 2 };
		const SDL_Rect entitySizeB = { pos.x + offsetSize.x - 2, pos.y + offsetSize.y - 2, size.x + 4, size.y + 4 };
		const SDL_Rect entitySizeC = { pos.x + offsetSize.x - 3, pos.y + offsetSize.y - 3, size.x + 6, size.y + 6 };
		App->printer->PrintQuad(entitySizeA, alphaColor);
		App->printer->PrintQuad(entitySizeB, alphaColor);
		App->printer->PrintQuad(entitySizeC, alphaColor);
	}
	if (isBlitSavedGroupSelection) {

		float alphaSpeed = 250.0f;
		alphaSavedGroupSelection -= alphaSpeed * dt;

		if (alphaSavedGroupSelection <= 0) {
			alphaSavedGroupSelection = 0;
			isBlitSavedGroupSelection = false;
		}

		SDL_Color alphaColor = color;
		alphaColor.a = alphaSavedGroupSelection;

		const SDL_Rect entitySize = { pos.x + offsetSize.x, pos.y + offsetSize.y, size.x, size.y };
		App->printer->PrintQuad(entitySize, alphaColor, true, true, Layers_FloorColliders);
	}
}

void Footman::Draw(SDL_Texture* sprites)
{
	if (animation != nullptr) {

		fPoint offset = { 0.0f,0.0f };

		if (animation == &footmanInfo.deathDown || animation == &footmanInfo.deathUp) {

			offset = { animation->GetCurrentFrame().w / 3.8f, animation->GetCurrentFrame().h / 3.3f };
			App->printer->PrintSprite({ (int)(pos.x - offset.x), (int)(pos.y - offset.y) }, sprites, animation->GetCurrentFrame(), Layers_FloorColliders);

			if (lifeBar != nullptr)
				if (lifeBar->isBlit)
					lifeBar->isBlit = false;
		}
		else {

			offset = { animation->GetCurrentFrame().w / 3.3f, animation->GetCurrentFrame().h / 3.3f };
			App->printer->PrintSprite({ (int)(pos.x - offset.x), (int)(pos.y - offset.y) }, sprites, animation->GetCurrentFrame(), Layers_Entities);

			if (lifeBar != nullptr)
				if (lifeBar->isBlit)
					lifeBar->isBlit = true;
		}
	}

	if (isSelected)
		DebugDrawSelected();
}

void Footman::DebugDrawSelected()
{
	const SDL_Rect entitySize = { pos.x + offsetSize.x, pos.y + offsetSize.y, size.x, size.y };
	App->printer->PrintQuad(entitySize, color);
}

void Footman::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{
	switch (collisionState) {

	case CollisionState_OnEnter:

		/// SET ATTACK PARAMETERS
		// An enemy unit/building is within the SIGHT RADIUS of this player unit
		if ((c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyUnit)
			|| (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_NeutralUnit)
			|| (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyBuilding)) {

			if (c2->entity == nullptr)
				return;

			if (c2->entity->entityType == EntityCategory_DYNAMIC_ENTITY) {

				DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
				dynEnt->SetLastSeenTile(App->map->WorldToMap(dynEnt->GetPos().x, dynEnt->GetPos().y));
			}

			/*
			if (isSelected) {

				DynamicEntity* dynEnt = (DynamicEntity*)c1->entity;
				LOG("Footman Sight Radius %s", dynEnt->GetColorName().data());
			}
			*/

			// 1. UPDATE TARGETS LIST
			list<TargetInfo*>::const_iterator it = targets.begin();
			bool isTargetFound = false;

			// If the target is already in the targets list, set its isSightSatisfied to true
			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isSightSatisfied = true;
					isTargetFound = true;
					break;
				}
				it++;
			}

			// Else, add the new target to the targets list (and set its isSightSatisfied to true)
			if (!isTargetFound) {

				TargetInfo* targetInfo = new TargetInfo();
				targetInfo->target = c2->entity;
				targetInfo->isSightSatisfied = true;

				targets.push_back(targetInfo);
			}

			// 2. MAKE UNIT FACE TOWARDS THE BEST TARGET
			if (targets.size() > 0) {

				bool isFacingTowardsTarget = false;

				// a) If the unit is not attacking any target
				if (currTarget == nullptr)
					isFacingTowardsTarget = true;

				if (isFacingTowardsTarget) {

					// Face towards the best target (ONLY DYNAMIC ENTITIES!) (it is expected to be this target)
					TargetInfo* targetInfo = GetBestTargetInfo(EntityCategory_DYNAMIC_ENTITY);

					if (targetInfo != nullptr) {

						fPoint orientation = { targetInfo->target->GetPos().x - pos.x, targetInfo->target->GetPos().y - pos.y };

						float m = sqrtf(pow(orientation.x, 2.0f) + pow(orientation.y, 2.0f));

						if (m > 0.0f) {
							orientation.x /= m;
							orientation.y /= m;
						}

						SetUnitDirectionByValue(orientation);
					}
				}
			}
		}

		// An enemy unit/building is within the ATTACK RADIUS of this player unit
		else if ((c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyUnit)
			|| (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_NeutralUnit)
			|| (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyBuilding)) {

			if (c2->entity == nullptr)
				return;

			/*
			if (isSelected) {

				DynamicEntity* dynEnt = (DynamicEntity*)c1->entity;
				LOG("Footman Attack Radius %s", dynEnt->GetColorName().data());
			}
			*/

			// 1. UPDATE TARGETS LIST
			list<TargetInfo*>::const_iterator it = targets.begin();
			bool isTargetFound = false;

			// If the target is already in the targets list, set its isAttackSatisfied + isSightSatisfied to true
			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isSightSatisfied = true;
					(*it)->isAttackSatisfied = true;
					isTargetFound = true;
					break;
				}
				it++;
			}
			// Else, add the new target to the targets list (and set its isAttackSatisfied + isSightSatisfied to true)
			if (!isTargetFound) {

				TargetInfo* targetInfo = new TargetInfo();
				targetInfo->target = c2->entity;
				targetInfo->isSightSatisfied = true;
				targetInfo->isAttackSatisfied = true;

				targets.push_back(targetInfo);
			}
		}
		break;

	case CollisionState_OnExit:

		/// RESET ATTACK PARAMETERS
		// An enemy unit/building is no longer within the SIGHT RADIUS of this player unit
		if ((c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyUnit)
			|| (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_NeutralUnit)
			|| (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyBuilding)) {

			if (c2->entity == nullptr)
				return;

			if (c2->entity->entityType == EntityCategory_DYNAMIC_ENTITY) {

				DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
				dynEnt->SetLastSeenTile(App->map->WorldToMap(dynEnt->GetPos().x, dynEnt->GetPos().y));
			}

			/*
			if (isSelected) {

				DynamicEntity* dynEnt = (DynamicEntity*)c1->entity;
				LOG("NO MORE Footman Sight Radius %s", dynEnt->GetColorName().data());
			}
			*/

			// Set the target's isSightSatisfied to false
			list<TargetInfo*>::iterator it = targets.begin();

			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isSightSatisfied = false;

					// Removing target process --
					if (!(*it)->IsTargetDead())

						(*it)->target->RemoveAttackingUnit(this);

					if (currTarget == *it)

						InvalidateCurrTarget();

					if ((*it)->isInGoals > 0 && !(*it)->isRemoveNeeded) {

						(*it)->isRemoveNeeded = true;
						targetsToRemove.splice(targetsToRemove.begin(), targets, it);
					}
					else if (!(*it)->isRemoveNeeded) {

						delete *it;
						targets.remove(*it);
					}
					// -- Removing target process

					break;
				}
				it++;
			}
		}

		// An enemy unit/building is no longer within the ATTACK RADIUS of this player unit
		else if ((c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyUnit)
			|| (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_NeutralUnit)
			|| (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyBuilding)) {

			if (c2->entity == nullptr)
				return;

			/*
			if (isSelected) {

				DynamicEntity* dynEnt = (DynamicEntity*)c1->entity;
				LOG("NO MORE Footman Attack Radius %s", dynEnt->GetColorName().data());
			}
			*/

			// Set the target's isAttackSatisfied to false
			list<TargetInfo*>::const_iterator it = targets.begin();

			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isAttackSatisfied = false;
					break;
				}
				it++;
			}
		}
		break;
	}
}

// State machine
void Footman::UnitStateMachine(float dt)
{
	switch (unitState) {

	case UnitState_Walk:
		
		if (IsUnitGatheringGold() || IsUnitHealingRunestone() || IsUnitRescuingPrisoner())
			break;

		// DEFENSE NOTE: the unit automatically attacks back their attacking units (if they have any attacking units) to defend themselves
		if (unitsAttacking.size() > 0) {

			if (singleUnit->IsFittingTile()) {

				// If the unit is not ordered to run away...
				if (!isRunAway) {

					if (currTarget == nullptr) {

						// Check if there are available targets (DYNAMIC ENTITY) 
						newTarget = GetBestTargetInfo(EntityCategory_DYNAMIC_ENTITY);

						if (newTarget != nullptr) {

							if (SetCurrTarget(newTarget->target))
								brain->AddGoal_AttackTarget(newTarget, false);

							newTarget = nullptr;
						}
					}
				}
			}
		}
			
		break;

	case UnitState_Idle:

		isRunAway = false;

		if (IsUnitGatheringGold() || IsUnitHealingRunestone() || IsUnitRescuingPrisoner())
			break;

		// If the unit is doing nothing, make it look around
		if (brain->GetSubgoalsList().size() == 0)

			brain->AddGoal_LookAround(1, 3, 1, 2, 2);

	case UnitState_Patrol:

		if (IsUnitGatheringGold() || IsUnitHealingRunestone() || IsUnitRescuingPrisoner())
			break;

		// ATTACK NOTE (Idle and Patrol states): the unit automatically attacks any target (only DYNAMIC ENTITIES) that is in their targets list

		if (singleUnit->IsFittingTile()) {

			// Check if there are available targets (DYNAMIC ENTITY)
			newTarget = GetBestTargetInfo(EntityCategory_DYNAMIC_ENTITY);

			if (newTarget != nullptr) {

				Room* room = App->map->GetEntityRoom(this);

				// A new target has found! Update the currTarget
				if (currTarget != newTarget && room != nullptr) {

					if (App->map->IsOnRoom(newTarget->target->GetPos(), *room)) {

						// Anticipate the removing of this unit from the attacking units of the target
						if (currTarget != nullptr)
							currTarget->target->RemoveAttackingUnit(this);

						isHitting = false;

						if (SetCurrTarget(newTarget->target))
							brain->AddGoal_AttackTarget(newTarget);

						newTarget = nullptr;
					}
				}
			}
		}

		break;

	case UnitState_AttackTarget:

		if (IsUnitGatheringGold() || IsUnitHealingRunestone() || IsUnitRescuingPrisoner())
			break;

		// ATTACK NOTE (Attack state): if currTarget is dead, the unit automatically attacks the next target (only DYNAMIC ENTITIES) from their targets list

		if (singleUnit->IsFittingTile()) {

			if (currTarget == nullptr) {

				// Check if there are available targets (DYNAMIC ENTITY) 
				newTarget = GetBestTargetInfo(EntityCategory_DYNAMIC_ENTITY);

				if (newTarget != nullptr) {

					if (SetCurrTarget(newTarget->target))
						brain->AddGoal_AttackTarget(newTarget);

					newTarget = nullptr;
				}
			}
		}

		break;

	case UnitState_HealRunestone:
	case UnitState_GatherGold:
	case UnitState_RescuePrisoner:

		break;

	case UnitState_Die:

		// Remove the corpse when a certain time is reached
		if (deadTimer.ReadSec() >= TIME_REMOVE_CORPSE)
			isRemove = true;

		break;

	case UnitState_NoState:
	default:

		break;
	}
}

// -------------------------------------------------------------

// Animations
void Footman::LoadAnimationsSpeed()
{
	upSpeed = footmanInfo.up.speed;
	downSpeed = footmanInfo.down.speed;
	leftSpeed = footmanInfo.left.speed;
	rightSpeed = footmanInfo.right.speed;
	upLeftSpeed = footmanInfo.upLeft.speed;
	upRightSpeed = footmanInfo.upRight.speed;
	downLeftSpeed = footmanInfo.downLeft.speed;
	downRightSpeed = footmanInfo.downRight.speed;

	attackUpSpeed = footmanInfo.attackUp.speed;
	attackDownSpeed = footmanInfo.attackDown.speed;
	attackLeftSpeed = footmanInfo.attackLeft.speed;
	attackRightSpeed = footmanInfo.attackRight.speed;
	attackUpLeftSpeed = footmanInfo.attackUpLeft.speed;
	attackUpRightSpeed = footmanInfo.attackUpRight.speed;
	attackDownLeftSpeed = footmanInfo.attackDownLeft.speed;
	attackDownRightSpeed = footmanInfo.attackDownRight.speed;

	deathUpSpeed = footmanInfo.deathUp.speed;
	deathDownSpeed = footmanInfo.deathDown.speed;
}

void Footman::UpdateAnimationsSpeed(float dt)
{
	footmanInfo.up.speed = upSpeed * dt;
	footmanInfo.down.speed = downSpeed * dt;
	footmanInfo.left.speed = leftSpeed * dt;
	footmanInfo.right.speed = rightSpeed * dt;
	footmanInfo.upLeft.speed = upLeftSpeed * dt;
	footmanInfo.upRight.speed = upRightSpeed * dt;
	footmanInfo.downLeft.speed = downLeftSpeed * dt;
	footmanInfo.downRight.speed = downRightSpeed * dt;

	footmanInfo.attackUp.speed = attackUpSpeed * dt;
	footmanInfo.attackDown.speed = attackDownSpeed * dt;
	footmanInfo.attackLeft.speed = attackLeftSpeed * dt;
	footmanInfo.attackRight.speed = attackRightSpeed * dt;
	footmanInfo.attackUpLeft.speed = attackUpLeftSpeed * dt;
	footmanInfo.attackUpRight.speed = attackUpRightSpeed * dt;
	footmanInfo.attackDownLeft.speed = attackDownLeftSpeed * dt;
	footmanInfo.attackDownRight.speed = attackDownRightSpeed * dt;

	footmanInfo.deathUp.speed = deathUpSpeed * dt;
	footmanInfo.deathDown.speed = deathDownSpeed * dt;
}

bool Footman::ChangeAnimation()
{
	bool ret = false;

	// The unit is dead
	if (isDead) 
	{
		if (animation != nullptr)
		{
			UnitDirection dir = GetUnitDirection();

			if (dir == UnitDirection_Up || dir == UnitDirection_Up || dir == UnitDirection_UpLeft || dir == UnitDirection_UpRight
				|| dir == UnitDirection_Left || dir == UnitDirection_Right || dir == UnitDirection_NoDirection) {

				if (animation->Finished() && unitState != UnitState_Die) {
					unitState = UnitState_Die;
					deadTimer.Start();
				}

				animation = &footmanInfo.deathUp;
				ret = true;
			}
			else if (dir == UnitDirection_Down || dir == UnitDirection_DownLeft || dir == UnitDirection_DownRight) {

				if (animation->Finished() && unitState != UnitState_Die) {
					unitState = UnitState_Die;
					deadTimer.Start();
				}

				animation = &footmanInfo.deathDown;
				ret = true;
			}
		}
		return ret;
	}

	// The unit is hitting their target
	else if (isHitting) {

		// Set the direction of the unit as the orientation towards the attacking target
		if (currTarget != nullptr) {

			fPoint orientation = { -1,-1 };

			if (currTarget->attackingTile.x != -1 && currTarget->attackingTile.y != -1) {

				iPoint attackingPos = App->map->MapToWorld(currTarget->attackingTile.x, currTarget->attackingTile.y);
				orientation = { attackingPos.x - pos.x, attackingPos.y - pos.y };
			}
			else
				orientation = { currTarget->target->GetPos().x - pos.x, currTarget->target->GetPos().y - pos.y };

			float m = sqrtf(pow(orientation.x, 2.0f) + pow(orientation.y, 2.0f));

			if (m > 0.0f) {
				orientation.x /= m;
				orientation.y /= m;
			}

			SetUnitDirectionByValue(orientation);
		}

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			animation = &footmanInfo.attackUp;
			ret = true;
			break;

		case UnitDirection_Down:

			animation = &footmanInfo.attackDown;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &footmanInfo.attackLeft;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &footmanInfo.attackRight;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &footmanInfo.attackUpLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &footmanInfo.attackUpRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &footmanInfo.attackDownLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &footmanInfo.attackDownRight;
			ret = true;
			break;
		}

		return ret;
	}

	// The unit is either moving or still
	else {

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			if (isStill) {

				footmanInfo.up.loop = false;
				footmanInfo.up.Reset();
				footmanInfo.up.speed = 0.0f;
			}
			else
				footmanInfo.up.loop = true;

			animation = &footmanInfo.up;

			ret = true;
			break;

		case UnitDirection_NoDirection:
		case UnitDirection_Down:

			if (isStill) {

				footmanInfo.down.loop = false;
				footmanInfo.down.Reset();
				footmanInfo.down.speed = 0.0f;
			}
			else
				footmanInfo.down.loop = true;

			animation = &footmanInfo.down;

			ret = true;
			break;

		case UnitDirection_Left:

			if (isStill) {

				footmanInfo.left.loop = false;
				footmanInfo.left.Reset();
				footmanInfo.left.speed = 0.0f;
			}
			else
				footmanInfo.left.loop = true;

			animation = &footmanInfo.left;

			ret = true;
			break;

		case UnitDirection_Right:

			if (isStill) {

				footmanInfo.right.loop = false;
				footmanInfo.right.Reset();
				footmanInfo.right.speed = 0.0f;
			}
			else
				footmanInfo.right.loop = true;

			animation = &footmanInfo.right;

			ret = true;
			break;

		case UnitDirection_UpLeft:

			if (isStill) {

				footmanInfo.upLeft.loop = false;
				footmanInfo.upLeft.Reset();
				footmanInfo.upLeft.speed = 0.0f;
			}
			else
				footmanInfo.upLeft.loop = true;

			animation = &footmanInfo.upLeft;

			ret = true;
			break;

		case UnitDirection_UpRight:

			if (isStill) {

				footmanInfo.upRight.loop = false;
				footmanInfo.upRight.Reset();
				footmanInfo.upRight.speed = 0.0f;
			}
			else
				footmanInfo.upRight.loop = true;

			animation = &footmanInfo.upRight;

			ret = true;
			break;

		case UnitDirection_DownLeft:

			if (isStill) {

				footmanInfo.downLeft.loop = false;
				footmanInfo.downLeft.Reset();
				footmanInfo.downLeft.speed = 0.0f;
			}
			else
				footmanInfo.downLeft.loop = true;

			animation = &footmanInfo.downLeft;

			ret = true;
			break;

		case UnitDirection_DownRight:

			if (isStill) {

				footmanInfo.downRight.loop = false;
				footmanInfo.downRight.Reset();
				footmanInfo.downRight.speed = 0.0f;
			}
			else
				footmanInfo.downRight.loop = true;

			animation = &footmanInfo.downRight;

			ret = true;
			break;
		}
		return ret;
	}
	return ret;
}