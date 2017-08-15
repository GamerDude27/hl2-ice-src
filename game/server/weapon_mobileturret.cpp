//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		MobileTurret - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "basebludgeonweapon.h"
#include "vstdlib/random.h"
#include "npcevent.h"
#include "ai_basenpc.h"

#include "gamestats.h"
#include "npc_mobile_turret.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	MOBILETURRET_RANGE	75.0f
#define	MOBILETURRET_REFIRE	0.4f

ConVar    sk_plr_dmg_mobileturret		( "sk_plr_dmg_mobileturret","0");
ConVar    sk_npc_dmg_mobileturret		( "sk_npc_dmg_mobileturret","0");

//-----------------------------------------------------------------------------
// CWeaponMobileTurret
//-----------------------------------------------------------------------------

class CWeaponMobileTurret : public CBaseHLBludgeonWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponMobileTurret, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CWeaponMobileTurret(void);

	float		GetRange( void )		{	return	MOBILETURRET_RANGE;	}
	float		GetFireRate( void )		{	return	MOBILETURRET_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	void	Precache( void );
	void	PrimaryAttack( void );	// drop the turret if in available position
	void	SecondaryAttack( void ); // use to bludgeon enemy
	bool	Reload( void );
	void	ItemPostFrame( void );
	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	// Animation event
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	//
	bool m_bRedraw;
};

//-----------------------------------------------------------------------------
// CWeaponMobileTurret
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CWeaponMobileTurret )
	DEFINE_FIELD( m_bRedraw, FIELD_BOOLEAN ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponMobileTurret, DT_WeaponMobileTurret)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_mobileturret, CWeaponMobileTurret );
PRECACHE_WEAPON_REGISTER( weapon_mobileturret );

acttable_t CWeaponMobileTurret::m_acttable[] = 
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },
};

IMPLEMENT_ACTTABLE(CWeaponMobileTurret);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponMobileTurret::CWeaponMobileTurret( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMobileTurret::Precache( void )
{
	UTIL_PrecacheOther("npc_mobile_turret");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponMobileTurret::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_mobileturret.GetFloat();

	return sk_npc_dmg_mobileturret.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponMobileTurret::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}

void CWeaponMobileTurret::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	// Cannot fire underwater
	if ( GetOwner() && GetOwner()->GetWaterLevel() == 3 )
	{
		SendWeaponAnim( ACT_VM_DRAW );
		//BaseClass::WeaponSound( EMPTY );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration( ACT_VM_DRAW );
		return;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition()-Vector(0,0,32); //<-- lowerd the release height
	Vector	vecThrow;
	// Don't autoaim on mobile turret tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 600.0f, vecThrow );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	//remove the ammo
	pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

	//
	// CREATE THE MOBILE TURRET
	//
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CNPC_Mobile_Turret *pMobileTurret = (CNPC_Mobile_Turret*)Create( "npc_mobile_turret", vecSrc, pPlayer->GetAbsAngles(), pPlayer );

	//WeaponSound( WPN_DOUBLE );    <- might want to play a tossing sound
	m_bRedraw = true;
	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
	m_flNextPrimaryAttack = gpGlobals->curtime + 2;
}

void CWeaponMobileTurret::SecondaryAttack( void )
{
	// bludgeon
	if ( m_bRedraw )
		return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMobileTurret::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMobileTurret::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMobileTurret::Deploy( void )
{
	m_bRedraw = false;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponMobileTurret::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponMobileTurret::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	/*
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if ( pEnemy )
	{
		Vector vecDelta;
		VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
		VectorNormalize( vecDelta );
		
		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize( vecDelta2D );
		if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
		{
			vecDirection = vecDelta;
		}
	}

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), sk_npc_dmg_mobileturret.GetFloat(), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
	*/
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponMobileTurret::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	/*
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}*/
}
