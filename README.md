# Midnight Cleanup

> 게임인재원 5기 최종 프로젝트 자체엔진

---

## 📺 프로젝트 소개 영상

아래는 프로젝트의 소개 영상입니다:

[![프로젝트 소개 영상](https://img.youtube.com/vi/WkjI6ZwUVwU/0.jpg)](https://www.youtube.com/watch?v=WkjI6ZwUVwU)

---

## 📋 요구사항

- **Visual Studio**: 2022
- **운영 체제**: Windows 10 또는 Windows 11
- **추가 패키지 매니저**: [vcpkg](https://github.com/microsoft/vcpkg)
---

## 🔨 빌드 방법

1. Visual Studio에서 `.sln` 파일을 엽니다.
2. (필요 시)Nuget 패키지를 복원합니다. *(아래 "Nuget 패키지 복원" 섹션 참고)*
3. 필요한 vcpkg 패키지를 설치합니다. *(아래 "vcpkg 패키지 설치" 섹션 참고)*
4. 추가 파일을 다운로드받아 설정합니다. *(아래 "추가 파일 다운로드" 섹션 참고)*
5. 프로젝트를 빌드하고 실행합니다.

---

## 📥 추가 파일 다운로드

이 프로젝트를 빌드하기 위해 추가적으로 필요한 파일을 다음 링크에서 다운로드받을 수 있습니다:

- [추가 파일 다운로드 링크](https://1drv.ms/f/s!AoSz5579eQ6GhymDO5ysipqE9MSd?e=Yc31Me)

### 파일 설정 방법
1. 위 링크에서 제공되는 파일을 다운로드합니다.
2. 다운로드한 파일을 프로젝트 폴더 내 적절한 위치에 복사합니다:
   - **MT_Opening.mp4**: `Resources/Video` 폴더 내에 위치하게 합니다.
   - **dll.zip** : 압축 해제, 솔루션 빌드 후 `x64` 폴더 내 `Debug` 혹은 `Release` 폴더 안에 복사 붙여넣기 합니다.
3. 파일 복사 후 프로젝트를 다시 빌드하여 실행합니다.

---

## 🛠️ Nuget 패키지 복원

이 프로젝트는 **Nuget 패키지**를 사용합니다.

1. Visual Studio에서 **Solution Explorer(솔루션 탐색기)**를 엽니다.
2. 솔루션(최상단)을 오른쪽 클릭합니다.
3. **Restore NuGet Packages**를 클릭합니다.

---

## 📦 vcpkg 패키지 설치

이 프로젝트는 추가적으로 `vcpkg`를 통해 패키지를 설치해야 합니다. 

### 설치 방법
1. [vcpkg 설치](https://github.com/microsoft/vcpkg) 페이지를 참고하여 `vcpkg`를 설치합니다.
2. 프로젝트 폴더에 있는 `vcpkg_list.txt` 파일을 확인합니다.
3. 아래 명령어를 사용하여 필요한 패키지를 설치합니다:
   ```bash
   vcpkg install <패키지명>

---

## 📝 참고 사항

- 시스템이 최소 요구 사항을 충족하는지 확인하세요.
- 빌드 과정에서 문제가 발생할 경우, 추가 파일 설정, NuGet 패키지 복원, 및 vcpkg 패키지 설치 과정을 다시 확인하세요.
- `vcpkg`가 Visual Studio 프로젝트와 올바르게 연동되었는지 확인하세요.

---

