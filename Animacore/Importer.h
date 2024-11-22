#pragma once

namespace core
{
	class Scene;

	// 타 엔진에서 읽은 데이터를 변환해주는 함수
	namespace Importer
	{
		/// Unity -> tool 로 유니티에 배치한 오브젝트들을 씬에 생성
		/// @param path Unity Parser 스크립트로 생성된 json 파일의 경로
		/// @param scene 오브젝트를 생성할 씬(기존 객체 파괴)
		void LoadUnityNew(const std::filesystem::path& path, core::Scene& scene);

		/// Unity -> tool 로 유니티에 배치한 오브젝트들을 씬에 생성
		/// @param path Unity Parser 스크립트로 생성된 json 파일의 경로
		/// @param scene 오브젝트를 생성할 씬(기존 객체 유지)
		void LoadUnityContinuous(const std::filesystem::path& path, core::Scene& scene);
	}
}
