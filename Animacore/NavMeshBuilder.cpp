#include "pch.h"
#include "NavMeshBuilder.h"

#include "Scene.h"
#include "NavConfigs.h"
#include "CoreSystemEvents.h"

#include <fstream>
#include <recastnavigation/DetourNavMesh.h>
#include <recastnavigation/DetourNavMeshBuilder.h>

core::NavMeshBuilder::NavMeshBuilder(Scene* scene)
	: _scene(scene)
{
	initialize();  // 필수 변수 초기화
}

core::NavMeshBuilder::~NavMeshBuilder()
{
	cleanup();
}

bool core::NavMeshBuilder::BuildNavMesh(const NavMeshSettings& settings, NavMeshData& data)
{
	// _vertices와 _indices는 InputMesh에서 설정된 데이터를 사용
	if (_vertices.empty() || _indices.empty()) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nNo mesh data available for NavMesh building.");
		return false;
	}

	// Step 1: 설정을 사용하여 config 초기화
	memset(&_config, 0, sizeof(_config));
	_config.cs = settings.cellSize;
	_config.ch = settings.cellHeight;
	_config.walkableSlopeAngle = settings.agentMaxSlope;
	_config.walkableHeight = static_cast<int>(ceilf(settings.agentHeight / _config.ch));
	_config.walkableClimb = static_cast<int>(floorf(settings.agentMaxClimb / _config.ch));
	_config.walkableRadius = static_cast<int>(ceilf(settings.agentRadius / _config.cs));
	_config.maxEdgeLen = static_cast<int>(settings.edgeMaxLen / _config.cs);
	_config.maxSimplificationError = settings.edgeMaxError;
	_config.minRegionArea = static_cast<int>(rcSqr(settings.regionMinSize));
	_config.mergeRegionArea = static_cast<int>(rcSqr(settings.regionMergeSize));
	_config.maxVertsPerPoly = static_cast<int>(settings.vertsPerPoly);
	_config.detailSampleDist = settings.detailSampleDist < 0.9f ? 0: _config.cs * settings.detailSampleDist;
	_config.detailSampleMaxError = _config.ch * settings.detailSampleMaxError;

	rcVcopy(_config.bmin, _bMin);
	rcVcopy(_config.bmax, _bMax);
	rcCalcGridSize(_config.bmin, _config.bmax, _config.cs, &_config.width, &_config.height);

	// Step 2: 높이 필드를 할당하고, 지형 데이터를 높이 필드로 변환
	_heightfield = rcAllocHeightfield();
	if (!_heightfield) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to allocate heightfield.");
		cleanup();
		return false;
	}

	if (!rcCreateHeightfield(_context, *_heightfield, _config.width, _config.height, _config.bmin, _config.bmax, _config.cs, _config.ch)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to create heightfield.");
		cleanup();
		return false;
	}

	// Step 3: 삼각형을 높이 필드로 레스터화
	std::vector<unsigned char> triAreas(_indices.size() / 3, RC_WALKABLE_AREA);
	rcMarkWalkableTriangles(_context, _config.walkableSlopeAngle, _vertices.data(), _vertices.size() / 3, _indices.data(), _indices.size() / 3, triAreas.data());
	rcRasterizeTriangles(_context, _vertices.data(), static_cast<int>(_vertices.size()) / 3,
		_indices.data(), triAreas.data(), static_cast<int>(_indices.size()) / 3, *_heightfield, _config.walkableClimb);

	// Step 4: 필터링을 통해 워크 가능 영역 정리
	rcFilterLowHangingWalkableObstacles(_context, _config.walkableClimb, *_heightfield);
	rcFilterLedgeSpans(_context, _config.walkableHeight, _config.walkableClimb, *_heightfield);
	rcFilterWalkableLowHeightSpans(_context, _config.walkableHeight, *_heightfield);

	// Step 5: Compact Heightfield 생성
	_compactHeightfield = rcAllocCompactHeightfield();
	if (!_compactHeightfield || !rcBuildCompactHeightfield(_context, _config.walkableHeight, _config.walkableClimb, *_heightfield, *_compactHeightfield)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to build compact heightfield.");
		cleanup();
		return false;
	}

	// 걷기 가능한 영역 침식
	if (!rcErodeWalkableArea(_context, _config.walkableRadius, *_compactHeightfield)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to erode walkable area.");
		cleanup();
		return false;
	}

	// 네비게이션 메쉬 생성 방식 (WATERSHED)
	{
		if (!rcBuildDistanceField(_context, *_compactHeightfield))
		{
			LOG_ERROR(*_scene, "NavMeshBuilder\nCould not build distance field.");
			return false;
		}

		if (!rcBuildRegions(_context, *_compactHeightfield, 0, _config.minRegionArea, _config.mergeRegionArea))
		{
			LOG_ERROR(*_scene, "NavMeshBuilder\nCould not build watershed regions.");
			return false;
		}
	}

	// Step 6: 윤곽선 생성 및 단순화
	_contourSet = rcAllocContourSet();
	if (!_contourSet || !rcBuildContours(_context, *_compactHeightfield, _config.maxSimplificationError, _config.maxEdgeLen, *_contourSet)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to create contours.");
		cleanup();
		return false;
	}

	// Step 7: Poly Mesh 생성
	_polyMesh = rcAllocPolyMesh();
	if (!_polyMesh || !rcBuildPolyMesh(_context, *_contourSet, _config.maxVertsPerPoly, *_polyMesh)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to build polymesh.");
		cleanup();
		return false;
	}

	// Step 8: Detail Mesh 생성
	_polyMeshDetail = rcAllocPolyMeshDetail();
	if (!_polyMeshDetail || !rcBuildPolyMeshDetail(_context, *_polyMesh, *_compactHeightfield, _config.detailSampleDist, _config.detailSampleMaxError, *_polyMeshDetail)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to build polymesh detail.");
		cleanup();
		return false;
	}

	// PolyFlags 설정
	for (int i = 0; i < _polyMesh->npolys; ++i) 
	{
		if (_polyMesh->areas[i] == RC_WALKABLE_AREA)
		{
			_polyMesh->flags[i] = 1;
		}
	}

	// Step 9: Detour 네비게이션 메쉬 생성
	dtNavMeshCreateParams params = {};
	params.verts = _polyMesh->verts;
	params.vertCount = _polyMesh->nverts;
	params.polys = _polyMesh->polys;
	params.polyAreas = _polyMesh->areas;
	params.polyFlags = _polyMesh->flags;
	params.polyCount = _polyMesh->npolys;
	params.nvp = _polyMesh->nvp;
	params.detailMeshes = _polyMeshDetail->meshes;
	params.detailVerts = _polyMeshDetail->verts;
	params.detailVertsCount = _polyMeshDetail->nverts;
	params.detailTris = _polyMeshDetail->tris;
	params.detailTriCount = _polyMeshDetail->ntris;
	params.walkableHeight = settings.agentHeight;
	params.walkableRadius = settings.agentRadius;
	params.walkableClimb = settings.agentMaxClimb;
	rcVcopy(params.bmin, _polyMesh->bmin);
	rcVcopy(params.bmax, _polyMesh->bmax);
	params.cs = _config.cs;
	params.ch = _config.ch;

	unsigned char* navData = nullptr;
	int navDataSize = 0;
	if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to create Detour navmesh.");
		cleanup();
		return false;
	}

	_navMesh = dtAllocNavMesh();
	if (!_navMesh || !_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)) {
		dtFree(navData);
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to initialize Detour navmesh.");
		cleanup();
		return false;
	}

	_navMeshQuery = dtAllocNavMeshQuery();
	if (!_navMeshQuery->init(_navMesh, 2048)) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to initialize Detour navmesh query.");
		cleanup();
		return false;
	}

	// NavMesh 및 관련 데이터를 NavMeshData에 저장
	data.navMesh = _navMesh;
	data.navMeshQuery = _navMeshQuery;

	return true;
}

void core::NavMeshBuilder::SaveNavMesh(std::filesystem::path path)
{
	if (!_navMesh) return;
	const auto* navMesh = _navMesh;

	// 파일 열기
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to open file for saving navmesh {}", path.string().c_str());
		return;
	}

	// NavMeshParams 및 타일 수를 저장
	const dtNavMeshParams* params = navMesh->getParams();
	file.write(reinterpret_cast<const char*>(params), sizeof(dtNavMeshParams));

	int numTiles = 0;
	for (int i = 0; i < navMesh->getMaxTiles(); ++i) {
		const dtMeshTile* tile = navMesh->getTile(i);
		if (tile && tile->header && tile->dataSize > 0) {
			numTiles++;
		}
	}
	file.write(reinterpret_cast<const char*>(&numTiles), sizeof(int));

	// 각 타일의 데이터를 저장
	for (int i = 0; i < navMesh->getMaxTiles(); ++i) {
		const dtMeshTile* tile = navMesh->getTile(i);
		if (tile && tile->header && tile->dataSize > 0) {
			file.write(reinterpret_cast<const char*>(tile->header), sizeof(dtMeshHeader));
			file.write(reinterpret_cast<const char*>(&tile->dataSize), sizeof(int));
			file.write(reinterpret_cast<const char*>(tile->data), tile->dataSize);
		}
	}

	LOG_INFO(*_scene, "NavMeshBuilder\nNavMesh saved to {}", path.string().c_str());
}

void core::NavMeshBuilder::LoadNavMesh(std::filesystem::path path, NavMeshData& data)
{
	// 파일 열기
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to open file for loading navmesh {}", path.string().c_str());
		return;
	}

	// NavMeshParams 읽기
	dtNavMeshParams params;
	file.read(reinterpret_cast<char*>(&params), sizeof(dtNavMeshParams));

	// NavMesh 할당 및 초기화
	_navMesh = dtAllocNavMesh();
	if (!_navMesh || dtStatusFailed(_navMesh->init(&params))) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to initialize navmesh from file {}", path.string().c_str());
		return;
	}

	// 타일 수 읽기
	int numTiles = 0;
	file.read(reinterpret_cast<char*>(&numTiles), sizeof(int));

	// 각 타일 읽기 및 NavMesh에 추가
	for (int i = 0; i < numTiles; ++i) {
		dtMeshHeader header;
		file.read(reinterpret_cast<char*>(&header), sizeof(dtMeshHeader));

		int dataSize = 0;
		file.read(reinterpret_cast<char*>(&dataSize), sizeof(int));

		unsigned char* data = (unsigned char*)malloc(dataSize);
		file.read(reinterpret_cast<char*>(data), dataSize);

		_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, nullptr);
	}

	// NavMeshQuery 초기화
	_navMeshQuery = dtAllocNavMeshQuery();
	if (dtStatusFailed(_navMeshQuery->init(_navMesh, 2048))) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to initialize navmesh query from file {}", path.string().c_str());
		return;
	}

	data.navMesh = _navMesh;
	data.navMeshQuery = _navMeshQuery;

	LOG_INFO(*_scene, "NavMeshBuilder\nNavMesh loaded from {}", path.string().c_str());
}

void core::NavMeshBuilder::InputWorld(const std::vector<float>& vertices, const std::vector<int>& indices)
{
	// 전달받은 vertices와 indices를 내부 변수로 저장
	_vertices = std::move(vertices);
	_indices = std::move(indices);

	calcMinMax();
}

void core::NavMeshBuilder::InputMesh(std::filesystem::path path)
{
	std::vector<float> vertices;
	std::vector<int> indices;

	// OBJ 파일을 읽어들이는 부분
	std::ifstream file(path);
	if (!file.is_open()) {
		LOG_ERROR(*_scene, "NavMeshBuilder\nFailed to open OBJ file, ", path.string());
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {  // 꼭짓점 데이터
			float x, y, z;
			iss >> x >> y >> z;
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);
		}
		else if (prefix == "f") {  // 삼각형 인덱스 데이터
			int v1, v2, v3;
			iss >> v1 >> v2 >> v3;
			indices.push_back(v1 - 1);  // OBJ 파일은 1-based 인덱스를 사용
			indices.push_back(v2 - 1);
			indices.push_back(v3 - 1);
		}
	}

	file.close();

	// 이후 BuildNavMesh에서 사용할 메쉬 데이터를 리턴
	_vertices = std::move(vertices);  // 클래스 멤버로 저장
	_indices = std::move(indices);    // 클래스 멤버로 저장

	calcMinMax();
}

void core::NavMeshBuilder::initialize()
{
	// Config 초기화
	memset(&_config, 0, sizeof(_config));

	// NavMeshSettings default 값을 적용
	NavMeshSettings settings;
	_config.cs = settings.cellSize;
	_config.ch = settings.cellHeight;
	_config.walkableSlopeAngle = settings.agentMaxSlope;
	_config.walkableHeight = (int)ceilf(settings.agentHeight / _config.ch);
	_config.walkableClimb = (int)floorf(settings.agentMaxClimb / _config.ch);
	_config.walkableRadius = (int)ceilf(settings.agentRadius / _config.cs);
	_config.maxEdgeLen = (int)(settings.edgeMaxLen / settings.cellSize);
	_config.maxSimplificationError = settings.edgeMaxError;
	_config.minRegionArea = (int)rcSqr(settings.regionMinSize);
	_config.mergeRegionArea = (int)rcSqr(settings.regionMergeSize);
	_config.maxVertsPerPoly = (int)settings.vertsPerPoly;
	_config.detailSampleDist = settings.detailSampleDist < 0.9f ? 0 : settings.cellSize * settings.detailSampleDist;
	_config.detailSampleMaxError = settings.cellHeight * settings.detailSampleMaxError;

	_context = new rcContext;
}

void core::NavMeshBuilder::cleanup()
{
	_vertices.clear();
	_indices.clear();

	if (_context)
	{
		delete _context;
		_context = nullptr;
	}
	if (_heightfield)
	{
		rcFreeHeightField(_heightfield);
		_heightfield = nullptr;
	}
	if (_compactHeightfield)
	{
		rcFreeCompactHeightfield(_compactHeightfield);
		_compactHeightfield = nullptr;
	}
	if (_contourSet)
	{
		rcFreeContourSet(_contourSet);
		_contourSet = nullptr;
	}
	if (_polyMesh)
	{
		rcFreePolyMesh(_polyMesh);
		_polyMesh = nullptr;
	}
	if (_polyMeshDetail)
	{
		rcFreePolyMeshDetail(_polyMeshDetail);
		_polyMeshDetail = nullptr;
	}
}

void core::NavMeshBuilder::calcMinMax()
{
	// bmin 및 bmax를 초기화
	_bMin[0] = _bMin[1] = _bMin[2] = FLT_MAX;
	_bMax[0] = _bMax[1] = _bMax[2] = -FLT_MAX;

	for (size_t i = 0; i < _vertices.size(); i += 3)
	{
		_bMin[0] = (std::min)(_bMin[0], _vertices[i]);
		_bMin[1] = (std::min)(_bMin[1], _vertices[i + 1]);
		_bMin[2] = (std::min)(_bMin[2], _vertices[i + 2]);

		_bMax[0] = (std::max)(_bMax[0], _vertices[i]);
		_bMax[1] = (std::max)(_bMax[1], _vertices[i + 1]);
		_bMax[2] = (std::max)(_bMax[2], _vertices[i + 2]);
	}
}
