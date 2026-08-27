// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCaptureComponent.generated.h"

class AAstrawildEchoBase;
class AAstrawildCaptureProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCaptureSuccessSignature, AActor*, TargetEcho, const FAstrawildCapturedEchoData&, EchoData, int32, PartySlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCaptureFailedSignature, AActor*, TargetEcho, int32, ShakesCompleted, const FText&, FailureReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCaptureFeedbackSignature, const FText&, FeedbackMessage, bool, bIsSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoSummonedSignature, AAstrawildEchoBase*, SummonedEcho);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEchoRecalledSignature);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildCaptureComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	TSubclassOf<AAstrawildCaptureProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float MaxCaptureRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Party", meta = (ClampMax = "5"))
	TArray<FAstrawildCapturedEchoData> ActiveParty;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
	TArray<FAstrawildCapturedEchoData> ReserveStorage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Party")
	int32 SelectedPartyIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	TWeakObjectPtr<AAstrawildEchoBase> ActiveSummonedEcho;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture")
	EAstrawildCaptureState CurrentCaptureState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture")
	FText LastCaptureFeedback;

	// --- Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Capture|Events")
	FOnCaptureSuccessSignature OnCaptureSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Capture|Events")
	FOnCaptureFailedSignature OnCaptureFailed;

	UPROPERTY(BlueprintAssignable, Category = "Capture|Events")
	FOnCaptureFeedbackSignature OnCaptureFeedback;

	UPROPERTY(BlueprintAssignable, Category = "Capture|Events")
	FOnEchoSummonedSignature OnEchoSummoned;

	UPROPERTY(BlueprintAssignable, Category = "Capture|Events")
	FOnEchoRecalledSignature OnEchoRecalled;

public:
	UFUNCTION(BlueprintCallable, Category = "Capture")
	bool ThrowResonator(float Power = 1.0f, FGameplayTag ResonatorItemTag = FGameplayTag());

	UFUNCTION(BlueprintPure, Category = "Capture")
	bool ValidateCapturePrerequisites(AAstrawildEchoBase* TargetEcho, FText& OutFailureReason) const;

	UFUNCTION(BlueprintCallable, Category = "Capture")
	bool AttemptCapture(AAstrawildEchoBase* TargetEcho, float ResonatorPower, int32& OutShakesCompleted, FText& OutStatusReason);

	UFUNCTION(BlueprintPure, Category = "Capture")
	float CalculateCaptureProbability(AAstrawildEchoBase* TargetEcho, float ResonatorPower) const;

	UFUNCTION(BlueprintPure, Category = "Capture")
	float CalculateInitialTrust(AAstrawildEchoBase* TargetEcho) const;

	UFUNCTION(BlueprintCallable, Category = "Companion")
	bool SummonSelectedCompanion();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void RecallActiveCompanion();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SelectNextPartySlot();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SelectPrevPartySlot();

	UFUNCTION(BlueprintCallable, Category = "Party")
	void AddCapturedEcho(const FAstrawildCapturedEchoData& InData);

	UFUNCTION(BlueprintCallable, Category = "Party")
	void LoadPartyData(const TArray<FAstrawildCapturedEchoData>& InParty, const TArray<FAstrawildCapturedEchoData>& InStorage);
};