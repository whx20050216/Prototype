// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/Enemy.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(10.0f);
	CollisionComp->SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	RootComponent = CollisionComp;

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // 确保阻挡玩家

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(CollisionComp);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
    TrailEffect->SetupAttachment(CollisionComp);

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 10000.0f;
	MovementComp->MaxSpeed = 10000.0f;
	MovementComp->ProjectileGravityScale = 0.0f;

	InitialLifeSpan = 5.0f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != GetInstigator())
	{
		// 范围伤害（RPG）或单体伤害（子弹）
		if (ExplosionRadius > 0.0f)
        {
            Explode();
        }
		else
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, UDamageType::StaticClass());
		}
		SpawnImpactEffect(Hit);
	}
	Destroy();
}

void AProjectile::Explode()
{
	// 范围伤害（RPG用）
    UGameplayStatics::ApplyRadialDamage(
        this, 
        Damage, 
        GetActorLocation(), 
        ExplosionRadius,
        UDamageType::StaticClass(),
        TArray<AActor*>(), // 忽略列表
        this, 
        GetInstigatorController(),
        true // 包含物理冲击
    );
}

void AProjectile::SpawnImpactEffect(const FHitResult& Hit)
{
}

