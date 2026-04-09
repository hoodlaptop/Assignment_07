# UE5 드론 비행체 구현 프로젝트

Unreal Engine 5 C++로 구현한 6자유도 드론 비행 시스템입니다.
지상 주행과 공중 비행을 모두 지원하며, 물리 기반 이동 시스템을 직접 구현했습니다.

---

## 조작 방법

| 키 | 동작 |
|----|------|
| W / S | 전진 / 후진 |
| A / D | 좌 / 우 |
| Space | 상승 |
| Shift | 하강 |
| 마우스 X | Yaw 회전 |
| 마우스 Y | Pitch 회전 |
| 마우스 휠 | Roll 회전 |

---

## 구현 기능

### 기본 구현

- `APawn` 상속 C++ 클래스
- `CapsuleComponent`를 RootComponent로 설정, `SkeletalMeshComponent`, `StaticMeshComponent`, `SpringArmComponent`, `CameraComponent` 계층 구성
- Enhanced Input System 기반 입력 처리 (`IA_Move`, `IA_Look`, `IA_Roll`)
- `ADroneController`에서 IMC 등록 및 관리
- `AddActorLocalRotation()`으로 Yaw / Pitch / Roll 직접 계산 구현

### 도전 구현

- 6자유도 이동 및 회전 (전/후/좌/우/상/하 + Yaw/Pitch/Roll)
- Pawn의 로컬 좌표계 기준 이동 (`GetActorForwardVector`, `GetActorRightVector`, `GetActorUpVector`)
- `Tick()`에서 중력 가속도 직접 계산 적용 (-980 cm/s²)
- `LineTrace`를 이용한 지면 감지 및 착지 시 Z 속도 초기화
- 지상/공중 상태 분기 및 공중 이동 속도 제한 (40%)

### 추가 구현 (과제 외)

#### 통합 물리 시스템

`FloatingPawnMovement` + 별도 `FallVelocity` 방식에서 단일 `FVector Velocity`로 통합했습니다.
수평과 수직 이동을 하나의 속도 벡터로 관리하여 두 시스템 간 충돌을 제거했습니다.

#### Damping 기반 감속

`MaxSpeed` 고정 제한 방식 대신 `FMath::Pow(Damping, DeltaTime)` 를 이용한 프레임 독립적 감쇠를 적용했습니다.
키를 뗐을 때 자연스럽게 감속하며, terminal velocity가 물리적으로 수렴합니다.

```cpp
Velocity *= FMath::Pow(Damping, DeltaTime);
```

#### 수평/수직 Sweep 분리

`AddActorWorldOffset` 단일 호출 시 수평 이동 중 바닥 Sweep 충돌이 전체 이동을 막는 문제를 해결하기 위해
수평과 수직 이동을 별도 Sweep으로 분리했습니다.

```cpp
AddActorWorldOffset(FVector(Velocity.X, Velocity.Y, 0.f) * DeltaTime, true, &HorizontalHit);
AddActorWorldOffset(FVector(0.f, 0.f, Velocity.Z) * DeltaTime, true, &VerticalHit);
```

#### 코드 기반 충돌 설정

```cpp
CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
CapsuleComp->SetCollisionObjectType(ECC_Pawn);
```

---

## 물리 파라미터 (에디터에서 조절 가능)

| 변수 | 기본값 | 설명 |
|------|--------|------|
| `GravityAccel` | -980 | 중력 가속도 (cm/s²) |
| `Acceleration` | 1500 | 입력 가속력 |
| `Damping` | 0.5 | 속도 감쇠 (0 = 즉시 정지, 1 = 감쇠 없음) |
| `AirAccelerationMultiplier` | 0.4 | 공중 수평 가속 비율 |

---

## 트러블슈팅

### IMC Swizzle 축 설정 오류
**증상**: A키를 누르면 아래로, Space키를 누르면 오른쪽으로 이동
**원인**: IMC에서 A키와 Shift키의 Swizzle Order가 뒤바뀌어 있었음
**해결**: 각 키의 Swizzle Order를 정정
- A / D → `YXZ` (X↔Y 교환)
- Space / Shift → `ZYX` (X↔Z 교환)

---

### AddMovementInput 무반응
**증상**: 키 입력 시 전혀 이동하지 않음
**원인**: `FloatingPawnMovement` 컴포넌트가 생성자에 추가되지 않아 `AddMovementInput`이 동작하지 않음
**해결**: 생성자에 `CreateDefaultSubobject<UFloatingPawnMovement>` 추가

---

### IMC 미등록
**증상**: 입력 이벤트 자체가 발화하지 않음
**원인**: `ADroneController::BeginPlay()`가 없어 IMC가 Enhanced Input 서브시스템에 등록되지 않음
**해결**: `BeginPlay()`에서 `UEnhancedInputLocalPlayerSubsystem::AddMappingContext()` 호출

---

### ZInput 미초기화
**증상**: Space를 한 번 누른 후 계속 상승
**원인**: `ETriggerEvent::Triggered`는 키를 뗄 때 호출되지 않아 `ZInput`이 1로 고정됨
**해결**: `ETriggerEvent::Completed` 바인딩 추가 후 `MoveInputVec = FVector::ZeroVector` 초기화

---

### 지면 관통
**증상**: 중력 적용 시 드론이 바닥을 뚫고 낙하
**원인 1**: `CapsuleComponent` 충돌 설정이 없어 Sweep이 감지 불가
**원인 2**: 착지 후 위치 보정 없이 속도만 0으로 초기화
**해결**: 코드에서 충돌 설정 명시 + `Hit.ImpactPoint.Z + HalfHeight`로 위치 스냅

---

### 착지 후 이륙 불가
**증상**: 지면 스냅 로직이 매 프레임 작동하여 Space를 눌러도 지면에 붙어 있음
**원인**: `bGrounded == true` 조건만으로 스냅을 적용하여 상승 시에도 강제로 스냅됨
**해결**: `bGrounded && Velocity.Z < 0.f` 조건 추가 — 낙하 중일 때만 스냅 적용

---

### 낙하 중 Space 재입력 무반응
**증상**: 낙하 중 Space를 눌러도 상승하지 않음
**원인**: `Velocity.Z`가 낙하하는 동안 크게 누적되어 ThrustForce로 극복하는 데 너무 오래 걸림
**해결**: `FMath::Max(Velocity.Z, -600.f)`로 최대 낙하 속도 제한 (terminal velocity)

---

### Space를 뗐을 때 과도한 상승 관성
**증상**: Space를 짧게 눌러도 의도보다 훨씬 높이 올라감
**원인**: Space를 누르는 동안 `Velocity.Z`가 계속 누적되어 키를 뗀 후에도 관성이 과도하게 남음
**해결**: `FMath::Clamp` → 최종적으로 `Damping` 방식으로 전환하여 자연스러운 감속 구현

---

### 지상 수평 이동 불가
**증상**: 지면에서 W/A/S/D를 눌러도 이동하지 않음
**원인**: `AddActorWorldOffset`에 수평+수직 속도를 합산한 벡터로 단일 Sweep 호출 시
바닥 충돌이 감지되면 수평 방향 이동도 함께 차단됨
**해결**: 수평/수직 `AddActorWorldOffset` 호출을 분리하여 각각 독립적으로 Sweep 처리
