// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GGGamesCharacter.generated.h"

class UTextRenderComponent;

/**
 * This class is the default character for GGGames, and it is responsible for all
 * physical interaction between the player and the world.
 *
 * The capsule component (inherited from ACharacter) handles collision with the world
 * The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
 * The Sprite component (inherited from APaperCharacter) handles the visuals
 */
UCLASS(config=Game)
class AGGGamesCharacter : public APaperCharacter
{
	GENERATED_BODY()

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	class UCameraComponent* SideViewCameraComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UTextRenderComponent* TextComponent;
	virtual void Tick(float DeltaSeconds) override;
protected:
	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animations)
	class UPaperFlipbook* RunningAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* AttackAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* DefenceAnimation;

	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();

	/** Called for side to side input */
	UFUNCTION(BlueprintCallable, Category = "GG Game")
	void GGMoveRight(float Value);

	UFUNCTION(BlueprintCallable, Category = "GG Game")
	void GGMoveUp(float Value);

	void UpdateCharacter();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GG Game")
	void GGAttack();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GG Game")
	void GGDefence();

	UFUNCTION(Server, Reliable)
	void ServerRequestAttack();

	UFUNCTION(Server, Reliable)
	void ServerRequestDefence();

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	AGGGamesCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

#pragma region GG Game

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GG Game")
	FVector m_vecGGCurrentForward = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GG Game")
	float m_fGGAttackVelocityStrength = 3000.0f;

	UPROPERTY(Replicated)
	bool m_bDuringGGAttack = false;

	UPROPERTY(Replicated)
	bool m_bDuringGGDefence = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GG Game")
	float m_fGGDefenceTime = 0.8f;

	float m_fGGDefenceTimeCounter = 0.0f;

	UFUNCTION(BlueprintPure, Category = "GG Game")
	FORCEINLINE bool IsDuringGGAttack() { return m_bDuringGGAttack; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GG Game")
	void SetDuringGGAttack(bool bGGAttack);

	UFUNCTION(BlueprintPure, Category = "GG Game")
	FORCEINLINE bool IsDuringGGDefence() { return m_bDuringGGDefence; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GG Game")
	void SetDuringGGDefence(bool bGGDefence);

	UFUNCTION(BlueprintPure, Category = "GG Game")
	FORCEINLINE bool IsDuringGGAction() { return m_bDuringGGAttack || m_bDuringGGDefence; }

#pragma endregion GG Game
};
