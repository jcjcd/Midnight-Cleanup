#pragma once
#include "RendererDLL.h"

class Renderer;
class RendererContext;
class Buffer;

class ANIMAVISION_DLL Shader
{
public:
	virtual ~Shader() = default;

	virtual void Bind(Renderer* renderer) = 0;
	virtual void Unbind(Renderer* renderer) {}
	virtual void BindContext(RendererContext* context) {}

	virtual void SetInt(const std::string& name, int value) = 0;
	virtual void SetIntArray(const std::string& name, int* value) = 0;
	virtual void SetFloat(const std::string& name, float value) = 0;
	virtual void SetFloat2(const std::string& name, const Vector2& value) = 0;
	virtual void SetFloat3(const std::string& name, const Vector3& value) = 0;
	virtual void SetFloat4(const std::string& name, const Vector4& value) = 0;
	virtual void SetMatrix(const std::string& name, const Matrix& value) = 0;
	virtual void SetStruct(const std::string& name, const void* value) = 0;
	virtual void SetConstant(const std::string& name, const void* value, uint32_t size) = 0;
	virtual void MapConstantBuffer(RendererContext* context) {}
	virtual void UnmapConstantBuffer(RendererContext* context) {}
	virtual void MapConstantBuffer(RendererContext* context, std::string bufName) {}
	virtual void UnmapConstantBuffer(RendererContext* context, std::string bufName) {}
	virtual Buffer* GetConstantBuffer(const std::string& name, int index) { return nullptr; }

	static std::shared_ptr<Shader> Create(Renderer* renderer, const std::string& srcPath);


	std::string Name;
	uint32_t ID = 0;
	bool IsValid = false;

	static inline uint32_t s_ShaderCount = 0;
};


class ShaderLibrary
{
public:
	ShaderLibrary() = default;
	~ShaderLibrary() = default;

	void AddShader(const std::shared_ptr<Shader>& shader);

	std::shared_ptr<Shader> LoadShader(Renderer* renderer, const std::string& srcPath);


	std::shared_ptr<Shader> GetShader(const std::string& name) const;

	std::map<std::string, std::shared_ptr<Shader>>& GetShaders() { return m_Shaders; }

	/// 이건 도저히 어떻게 해 낼수가 없었다..
	void ResetAllocateState4DX12();

private:
	std::map<std::string, std::shared_ptr<Shader>> m_Shaders;
};


