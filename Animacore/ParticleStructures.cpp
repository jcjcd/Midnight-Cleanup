#include "pch.h"
#include "ParticleStructures.h"
#include "RenderComponents.h"

#include "../Animavision/Renderer.h"

core::ParticleObject::ParticleObject(Renderer& renderer, uint32_t maxParticleCounts, Matrix transform, uint32_t entity)
{
	instanceData.transform = transform;

	using ParticleBuffers = std::tuple<std::shared_ptr<Texture>, std::shared_ptr<Texture>, std::shared_ptr<Texture>>;

	ParticleBuffers buffers = renderer.CreateParticleBufferTextures(maxParticleCounts, entity);

	particleBufferTexture = std::get<0>(buffers);
	deadListBufferTexture = std::get<1>(buffers);
	aliveListBufferTexture = std::get<2>(buffers);

	instanceData.randomSeed = renderer.GetRandomSeed(0.0f, 1.0f);
}

void core::ParticleObject::Reset()
{
	instanceData.timePos = 0.0f;
	instanceData.frameTime = 0.0f;
	accumulation = 0.0f;
	instanceData.numToEmit = 0;
}

void core::ParticleObject::CalculateNumToEmit(const ParticleSystem& data)
{
	float ft = instanceData.frameTime * data.mainData.simulationSpeed;

	float prevModTimePos = fmod(instanceData.timePos, data.mainData.duration);
	instanceData.timePos += ft;
	float modTimePos = fmod(instanceData.timePos, data.mainData.duration);

	if (data.emissionData.particlePerSecond < 0.0f)
		return;

	if (!data.mainData.isLooping && data.mainData.duration < instanceData.timePos)
		return;

	if (cycleCount < instanceData.timePos / data.mainData.duration)
	{
		cycleCount = static_cast<uint32_t>(instanceData.timePos / data.mainData.duration);
		isRunDurationCycle = true;
	}

	if (isRunDurationCycle)
	{
		switch (data.mainData.startDelayOption)
		{
			case ParticleSystem::Option::Constant:
				startTimePos = data.mainData.startDelay.x;
				break;
			case ParticleSystem::Option::RandomBetweenTwoConstants:
			{
				float randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				float ratio = [randF](float a, float b) { return a + (b - a) * randF; }(0.0f, 1.0f);
				startTimePos = std::lerp(data.mainData.startDelay.x, data.mainData.startDelay.y, ratio);
			}
			break;
			default:
				assert(false && "Invalid start delay option");
				break;
		}

		isRunDurationCycle = false;
	}

	if (startTimePos > modTimePos)
		return;

	if (data.instanceData.isEmit)
		accumulation += data.emissionData.particlePerSecond * ft;

	if (accumulation > 1.0f)
	{
		double integerPart;

		accumulation = static_cast<float>(std::modf(accumulation, &integerPart));
		instanceData.numToEmit = static_cast<uint32_t>(integerPart);
	}
	else
		instanceData.numToEmit = 0;

	for (auto& burst : data.emissionData.bursts)
	{
		for (uint32_t i = 0; i < burst.cycles; i++)
		{
			float checkedTimePos = burst.timePos + burst.interval * i;

			if (prevModTimePos < checkedTimePos && checkedTimePos < modTimePos)
			{
				float randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
				float ratio = [randF](float a, float b) { return a + (b - a) * randF; }(0.0f, 1.0f);

				if (burst.probability >= ratio)
					instanceData.numToEmit += burst.count;
			}
		}
	}
}

/// WOO 먼지 이펙트 만들 때 다시 수정
/// 현재 문제 1. 한 번 애니메이션이 다 돌고 루프할 때 제일 첫번째 텍스처 시트로 안 간다
/// 현재 문제 2. 일정한 애니메이션이 아니다
void core::ParticleObject::UpdateTextureSheetAnimation(const ParticleSystem& data)
{
	float animationIter = data.mainData.duration / (data.textureSheetAnimationData.tiles.x * data.textureSheetAnimationData.tiles.y);

	// 한 animationIter이 지날 때마다 uv값을 변경
	// x * y개
	// x개가 한 줄

	uint32_t xNum = static_cast<uint32_t>(data.textureSheetAnimationData.tiles.x);
	uint32_t yNum = static_cast<uint32_t>(data.textureSheetAnimationData.tiles.y);
	Matrix tilling = Matrix::CreateScale(1 / data.textureSheetAnimationData.tiles.x, 1 / data.textureSheetAnimationData.tiles.y, 0);
	renderData.texTransform = tilling;

	for (uint32_t i = 0; i < yNum; i++)
	{
		for (uint32_t j = 0; j < xNum; j++)
		{
			if (instanceData.timePos < animationIter)
			{
				renderData.texTransform = tilling;
			}
			else if (instanceData.timePos > animationIter * (xNum + yNum - 1))
			{
				Matrix offset = Matrix::CreateTranslation((xNum - 1) / data.textureSheetAnimationData.tiles.x,( yNum - 2 )/ data.textureSheetAnimationData.tiles.y, 0);
				renderData.texTransform = tilling * offset;
			}
			else
			{
				if (instanceData.timePos > animationIter * (i + j) && instanceData.timePos < animationIter * (i + j + 1))
				{
					uint32_t xIndex = j > 1 ? j - 1 : 0;
					uint32_t yIndex = i > 1 ? i - 1 : 0;

					Matrix offset = Matrix::CreateTranslation(xIndex / data.textureSheetAnimationData.tiles.x, yIndex / data.textureSheetAnimationData.tiles.y, 0);
					renderData.texTransform = tilling * offset;
				}
			}
		}
	}
}
