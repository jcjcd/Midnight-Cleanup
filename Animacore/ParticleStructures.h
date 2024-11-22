#pragma once

class Texture;
class Renderer;

namespace core
{
	struct ParticleSystem;

	struct Particle
	{
		Vector3 PositionW;
		float Size;

		Vector3 VelocityW;
		float Rotation;

		Vector4 StartColor;
		Vector4 Color;

		Vector3 EmitterPosition;
		float GravityModifier;

		float LifeTime;
		float Age;
	};

	struct ParticleObject
	{
		ParticleObject(Renderer& renderer, uint32_t maxParticleCounts, Matrix transform, uint32_t entity);
		ParticleObject() = default;
		void Reset();
		void CalculateNumToEmit(const ParticleSystem& data);
		void SetFrameTime(float frameTime) { instanceData.frameTime = frameTime; }
		void UpdateTextureSheetAnimation(const ParticleSystem& data);

		bool isFirstTime = true;

		float accumulation = 0.0f;

		bool isRunDurationCycle = false;
		float startTimePos = 0.0f;
		uint32_t cycleCount = 0;

		std::shared_ptr<Texture> particleBufferTexture;
		std::shared_ptr<Texture> deadListBufferTexture;
		std::shared_ptr<Texture> aliveListBufferTexture;

		struct Instance
		{
			Matrix transform{};
			float timePos = 0.0f;
			float frameTime = 0.0f;
			int numToEmit = 0;
			float randomSeed{};

		} instanceData;

		struct Main
		{
			Vector4 startColor[2]{};

			float startLifeTime[2]{};
			float startSpeed[2]{};

			float startRotation[2]{};
			float startSize[2]{};

			float gravityModifier[2]{};
			float simulationSpeed{};
			int maxParticleCount{};

		} mainData;

		struct Shape
		{
			Matrix transform{};
			Vector3 position{};
			float pad0{};
			Vector3 rotation{};
			float pad1{};
			Vector3 scale{};
			float pad2{};

			int shapeType{};
			int modeType{};
			float angleInRadian{};
			float radius{};

			float donutRadius{};
			float arcInRadian{};
			float spread{};

			float radiusThickness{};

		} shapeData;

		struct VelocityOverLifeTime
		{
			Vector3 velocity{};
			float pad0{};
			Vector3 orbital{};
			float pad1{};
			Vector3 offset{};
			float pad2{};
			int isUsed{};
			float pads[3]{};

		} velocityOverLifeTimeData;

		struct LimitVelocityOverLifeTime
		{
			float speed{};
			float dampen{};
			int isUsed = false;
			float pads[1]{};

		} limitVelocityOverLifeTimeData;

		struct ForceOverLifeTime
		{
			Vector3 force{};
			int isUsed = false;

		} forceOverLifeTimeData;

		struct ColorOverLifeTime
		{
			Vector4 colorRatios[8]{};
			Vector4 alphaRatios[8]{};
			int alphaRatioCount{};
			int colorRatioCount{};
			int isUsed = false;
			float pads[1]{};

		} colorOverLifeTimeData;

		struct SizeOverLifeTime
		{
			Vector2 pointA{};
			Vector2 pointB{};
			Vector2 pointC{};
			Vector2 pointD{};
			int isUsed = false;
			float pads[3]{};

		} sizeOverLifeTimeData;

		struct RotationOverLifeTime
		{
			float angularVelocityInRadian{};
			int isUsed = false;
			float pads[2];

		} rotationOverLifeTimeData;

		struct Render
		{
			Vector4 baseColor{};
			Vector4 emissiveColor{};
			Matrix texTransform{};

			int renderMode{};
			int colorMode{};
			int useAlbedoMap{};
			int useEmissiveMap{};

			int useMultiplyAlpha{};
			float alphaCutOff{};
			float pads[2]{};
		} renderData;
	};
}