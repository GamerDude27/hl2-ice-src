//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_HOGM_H
#define WEAPON_HOGM_H
#ifdef _WIN32
#pragma once
#endif



//-----------------------------------------------------------------------------
// Do we have the super-phys gun?
//-----------------------------------------------------------------------------
bool PlayerHasMegaHogm();

// force the Hogm to drop an object (if carried)
void HogmForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis );
void HogmBeginUpgrade( CBaseAnimating *pAnim );

bool PlayerPickupControllerIsHoldingEntity2( CBaseEntity *pPickupController, CBaseEntity *pHeldEntity );
float PlayerPickupGetHeldObjectMass2( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject );
float HogmGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject );

CBaseEntity *HogmGetHeldEntity( CBaseCombatWeapon *pActiveWeapon );
CBaseEntity *GetPlayerHeldEntity2( CBasePlayer *pPlayer );

bool HogmAccountableForObject( CBaseCombatWeapon *pHogm, CBaseEntity *pObject );

#endif // WEAPON_HOGM_H
