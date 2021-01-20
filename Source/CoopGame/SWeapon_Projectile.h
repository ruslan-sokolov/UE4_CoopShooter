// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SWeapon_Projectile.generated.h"

/**
 * 
 */

class ASProjectile;

UCLASS()
class COOPGAME_API ASWeapon_Projectile : public ASWeapon
{
	GENERATED_BODY()

	ASWeapon_Projectile();

public:
	/** Ammo class to use **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		TSubclassOf<ASProjectile> Projectile;

protected:
	virtual void FireLogic() override;
};
