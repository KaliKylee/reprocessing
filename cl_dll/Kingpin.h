#pragma once

#define KINGPIN_MAX_FAKEMONS 5

void SpawnKingpinEffects(void);
Vector RandomVectorForFake(float min, float max);
Vector GetRandomFakeGrenadeThrow(const Vector& throwDirection, float basePower, float spread);
void SpawnFakeGrenadeTE(void);
void SpawnFakeExpl(void);
void SpawnFakeMonsterTE(const char* modelName);
void UpdateFakeMons(void);