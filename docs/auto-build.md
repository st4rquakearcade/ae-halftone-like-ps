# 클릭만으로 설치 파일 받기 (GitHub 자동 빌드)

코딩 지식 없이, **GitHub가 대신 빌드**하게 해서 맥/윈도우용 완성 플러그인 파일을
받는 방법입니다. 딱 한 번 AE SDK를 GitHub에 올려두면, 그 다음부터는 버튼만 누르면
됩니다.

전체 흐름: **① SDK 받기 → ② SDK를 GitHub에 올리기 → ③ 빌드 버튼 누르기 → ④ 결과 파일 다운로드 → ⑤ 설치**

---

## ① After Effects SDK 받기 (최초 1회)

1. <https://developer.adobe.com/after-effects/> 접속
2. **Download the After Effects SDK** 버튼으로 SDK `.zip` 파일을 다운로드
   (로그인을 요구하면 무료 Adobe 계정으로 로그인)
3. 다운로드한 `.zip` 파일은 **압축을 풀지 말고 그대로** 둡니다.

> SDK는 Adobe 소유라 저장소에 같이 넣어둘 수 없어, 본인이 한 번 올려주셔야 합니다.

## ② SDK를 GitHub에 올리기 (최초 1회)

1. 브라우저에서 이 저장소 페이지로 갑니다:
   `https://github.com/st4rquakearcade/ae-halftone-like-ps`
2. 오른쪽 또는 상단의 **Releases** 클릭 → **Draft a new release** (또는 "Create a new release")
3. **Choose a tag** 칸에 `sdk` 라고 입력하고 **Create new tag: sdk** 선택
4. 아래 **Attach binaries...** 영역에 ①에서 받은 SDK `.zip` 파일을 **드래그**해서 올립니다
   (업로드 끝날 때까지 잠시 기다리기)
5. **Publish release** 클릭

이제 GitHub가 SDK를 가지고 있습니다. (이 단계는 SDK가 새 버전으로 바뀌지 않는 한 다시 안 해도 됩니다.)

## ③ 빌드 버튼 누르기

1. 저장소 상단 **Actions** 탭 클릭
2. 왼쪽 목록에서 **Build AE Plugin** 선택
3. 오른쪽 **Run workflow** 버튼 클릭 → 브랜치는
   `claude/ae-halftone-edge-tear-plugin-48hnwa` 선택 → **Run workflow**
4. 잠시 후 목록에 실행 항목이 생깁니다. 클릭해서 들어가 **초록 체크(✓)** 가 뜰 때까지 기다립니다
   (보통 몇 분).

## ④ 결과 파일 다운로드

빌드가 끝나면 두 곳 중 **편한 곳**에서 받으면 됩니다.

### (가장 쉬움) Releases 페이지에서 받기
빌드가 성공하면 결과 파일이 자동으로 **Releases**에 올라갑니다.

1. 저장소 오른쪽의 **Releases** → **Halftone & Torn Edges — 최신 빌드** 클릭
2. **Assets** 에서 본인 OS에 맞는 zip 다운로드:
   - 맥: `HalftoneTornEdges-macOS.zip`
   - 윈도우: `HalftoneTornEdges-Windows.zip`

이 방법은 Actions 탭을 안 거쳐도 되고, 항상 최신 빌드가 올라가 있습니다.

### (대안) Actions의 Artifacts에서 받기
1. 방금 그 실행(run) 페이지에서 **왼쪽 위 `Summary`** 클릭
2. 페이지 맨 아래 **Artifacts** 영역에 두 개가 있습니다:
   - `HalftoneTornEdges-macOS` ← 맥용
   - `HalftoneTornEdges-Windows` ← 윈도우용
3. 본인 OS에 맞는 것을 클릭해 다운로드

압축을 풀면:
- 맥: `HalftoneTornEdges.plugin`
- 윈도우: `HalftoneTornEdges.aex`

## ⑤ 설치

### 맥
```bash
# 플러그인 폴더로 복사 ("2024"는 본인 AE 버전으로)
cp -R ~/Downloads/HalftoneTornEdges.plugin "/Applications/Adobe After Effects 2024/Plug-ins/"

# macOS 차단(격리) 해제 — 안 하면 AE에 안 뜹니다
sudo xattr -dr com.apple.quarantine "/Applications/Adobe After Effects 2024/Plug-ins/HalftoneTornEdges.plugin"
```

> **"Apple은 ... 악성 코드가 없음을 확인할 수 없습니다" 라고 뜨면?**
> 악성코드라서가 아니라 **서명만 안 된** 플러그인이라 그렇습니다(직접 빌드한 플러그인은 다
> 이렇습니다). 위 `xattr` 명령이 이 차단을 풀어줍니다. 터미널이 어렵다면:
> **시스템 설정 → 개인정보 보호 및 보안** 으로 가서 아래로 스크롤하면
> "HalftoneTornEdges.plugin이(가) 차단되었습니다" 옆에 **"확인 없이 허용"** 버튼이 있습니다.
> 그걸 누른 뒤 AE를 재시작하세요.

### 윈도우
`HalftoneTornEdges.aex` 파일을 아래 폴더에 복사:
```
C:\Program Files\Adobe\Adobe After Effects <버전>\Support Files\Plug-ins\
```

마지막으로 **After Effects를 재시작** → 레이어 선택 →
**Effect → Stylize → Halftone & Torn Edges**.

---

## 잘 안 될 때

- **③에서 빨간 X(실패)가 뜨고 "AE SDK not found"** → ②의 릴리스 태그가 정확히 `sdk` 인지,
  `.zip` 파일이 제대로 첨부됐는지 확인하세요.
- **빌드 로그에 컴파일 에러** → 그 에러 메시지를 저에게 붙여주시면 소스를 고쳐 다시 올리겠습니다.
  (이 플러그인은 아직 실제 AE 환경에서 검증되지 않아, 첫 빌드에서 SDK 버전 차이로 사소한
  수정이 필요할 수 있습니다.)
- **AE에 효과가 안 보임 (맥)** → ⑤의 `xattr` 명령을 플러그인 경로에 정확히 실행했는지, 또는
  시스템 설정에서 "확인 없이 허용"을 눌렀는지 확인.
- **빌드 로그에 노란색 "Node.js 20 is deprecated" 경고** → 빨간 X(실패)가 아니라 단순 안내
  경고입니다. 빌드 성공/결과물에 아무 영향이 없으니 **무시해도 됩니다**.

### (대안) SDK를 URL로 주기
릴리스 업로드 대신, SDK zip의 직접 다운로드 URL이 있다면 저장소
**Settings → Secrets and variables → Actions → New repository secret** 에서
이름 `AESDK_URL`, 값에 그 URL을 넣어도 됩니다. 그러면 ②를 건너뛸 수 있습니다.
