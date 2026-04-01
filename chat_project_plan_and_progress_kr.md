# Chat Project Plan and Progress

## 1. 프로젝트 개요

이 프로젝트는 다음 기술 스택을 기반으로 **간단한 채팅 애플리케이션**을 만드는 것을 목표로 합니다.

- **서버**: C++17, Boost.Asio, TCP 소켓 통신
- **클라이언트**: C#, WPF 데스크톱 애플리케이션
- **개발 환경**: Windows, Visual Studio + CMake 중심

이 프로젝트의 목적은 단순히 채팅 기능을 구현하는 것에 그치지 않고, 다음과 같은 역량을 포트폴리오 형태로 보여주는 데 있습니다.

- 비동기 네트워크 프로그래밍 이해 및 구현
- 세션 및 연결 생명주기 관리
- 클라이언트-서버 프로토콜 설계
- WPF 기반 데스크톱 UI 구현
- MVP에서 확장형 구조로 점진적으로 발전시키는 설계 능력

---

## 2. 프로젝트 목표

### MVP 목표

가장 먼저 완성할 최소 기능 버전은 다음을 지원하는 것을 목표로 합니다.

- 서버가 TCP 포트를 열고 대기한다.
- 여러 클라이언트가 서버에 접속할 수 있다.
- 클라이언트가 닉네임을 등록할 수 있다.
- 클라이언트가 메시지를 전송할 수 있다.
- 서버가 접속 중인 클라이언트들에게 메시지를 브로드캐스트한다.
- 입장 / 퇴장 시스템 메시지를 표시할 수 있다.
- WPF 클라이언트에서 연결, 전송, 로그 확인이 가능하다.

### 확장 목표

MVP가 안정화되면 다음 기능을 추가로 고려합니다.

- 접속자 목록 표시
- 귓속말 / 개인 메시지
- 여러 채팅방 지원
- 메시지 타임스탬프
- JSON 기반 프로토콜
- 재접속 처리
- 서버 로그 저장
- 비정상 종료 감지

---

## 3. 개발 전략

추천 구현 순서는 다음과 같습니다.

1. **Boost.Asio 서버 골격부터 먼저 구현**
2. **간단한 테스트 클라이언트나 콘솔 TCP 도구로 통신 검증**
3. **세션 관리와 브로드캐스트 로직 추가**
4. **그 다음에 WPF 클라이언트 구현**
5. **구조 리팩터링 및 확장 기능 추가**

이 순서가 중요한 이유는, WPF를 너무 일찍 만들기 시작하면 프로토콜 변경이 발생할 때 UI 작업을 반복하게 될 가능성이 크기 때문입니다.

---

## 4. 초기 프로토콜 기획

초기 단계에서는 구현 복잡도를 낮추기 위해 **줄 단위 텍스트 프로토콜**을 사용합니다.

### 클라이언트 → 서버

```text
NICK|UserA
MSG|안녕하세요
```

### 서버 → 클라이언트

```text
SYSTEM|UserA joined
CHAT|UserA|안녕하세요
SYSTEM|UserA left
```

### 계획된 메시지 타입

- `NICK|{nickname}`
- `MSG|{message}`
- `SYSTEM|{message}`
- `CHAT|{nickname}|{message}`

MVP가 안정화되면 이후 JSON 프로토콜로 확장할 수 있습니다.

---

## 5. 계획한 프로젝트 구조

```text
ChatApp/
├─ server/
│  ├─ CMakeLists.txt
│  ├─ include/
│  │  ├─ chat_server.h
│  │  └─ client_session.h
│  └─ src/
│     ├─ main.cpp
│     ├─ chat_server.cpp
│     └─ client_session.cpp
└─ client/
   └─ ChatClient.Wpf/
```

---

## 6. 현재까지 수행한 내용

### 6.1 서버 골격 구현

최소한의 Boost.Asio TCP 서버 구조를 만들었습니다.

구현한 파일은 다음과 같습니다.

- `main.cpp`
- `chat_server.h`
- `chat_server.cpp`
- `client_session.h`
- `client_session.cpp`

### 6.2 서버 시작 흐름 구현

현재 서버의 시작 흐름은 다음과 같습니다.

1. `boost::asio::io_context` 생성
2. `work_guard` 생성
3. `ChatServer` 객체 생성
4. `9000` 포트에서 listen 시작
5. `io_context.run()` 호출

### 6.3 Accept 로직 구현

현재 서버는 다음 동작을 수행합니다.

- 지정된 포트에서 acceptor를 연다.
- 비동기적으로 클라이언트 접속을 기다린다.
- 접속이 발생하면 클라이언트 endpoint 정보를 출력한다.
- 해당 소켓으로 `ClientSession` 객체를 생성한다.
- 세션을 즉시 시작한다.
- 다음 접속을 위해 다시 async accept를 등록한다.

### 6.4 세션 수신 로직 구현

각 클라이언트 세션은 현재 다음을 수행합니다.

- 자신의 TCP 소켓을 소유한다.
- 비동기 줄 단위 read loop를 시작한다.
- `async_read_until(..., '\n')`를 사용한다.
- stream buffer에서 한 줄씩 읽는다.
- 수신한 메시지를 콘솔에 출력한다.
- 연결 종료 또는 에러가 발생할 때까지 read loop를 계속 이어간다.

### 6.5 현재까지의 도달 수준

현재 단계에서 다음 마일스톤까지 도달했습니다.

- 서버 실행 가능
- 클라이언트 접속 수락 가능
- 클라이언트가 보낸 한 줄 텍스트 수신 가능
- 연결 종료 및 read error 감지 가능

즉, 현재는 **"수신 전용 서버 MVP 골격"** 단계까지 구현된 상태입니다.

---

## 7. 개발 중 발생한 문제

### 7.1 증상

클라이언트가 접속하면 서버에서 다음과 같은 로그가 출력되었습니다.

```text
[Server] Accept Handler Error : bad_weak_ptr
```

### 7.2 원인

문제의 원인은 `client_session.h`의 다음 선언에 있었습니다.

```cpp
class ClientSession : std::enable_shared_from_this<ClientSession>
```

C++에서 `class`는 상속 접근 지정자의 기본값이 **private** 이기 때문에, `enable_shared_from_this`가 private 상속된 상태였습니다.

그 결과 `std::make_shared<ClientSession>(...)`로 객체를 생성했더라도, `shared_from_this()`가 의도대로 동작하기 위한 내부 weak ownership 연결이 올바르게 잡히지 않았습니다.

그리고 `DoRead()` 안의 다음 코드에서 예외가 발생했습니다.

```cpp
auto self = shared_from_this();
```

### 7.3 해결 방법

상속을 반드시 **public 상속**으로 바꿔야 합니다.

```cpp
class ClientSession : public std::enable_shared_from_this<ClientSession>
```

이렇게 수정하면 비동기 핸들러 내부에서 `shared_from_this()`를 안전하게 사용할 수 있습니다.

---

## 8. 디버깅 과정에서 정리된 추가 사항

안정화를 진행하면서 다음 사항들도 중요하다는 점을 확인했습니다.

- `work_guard`를 사용해서 `io_context.run()`이 예상치 않게 종료되지 않도록 해야 한다.
- async handler 내부 예외가 앱 종료로 이어지지 않도록 주의해야 한다.
- `#pragma once`는 `.cpp` 파일이 아니라 헤더 파일에만 사용하는 것이 맞다.

---

## 9. 현재 상태 요약

현재 구현은 아직 **완전한 채팅 서버**는 아니지만, 충분히 좋은 기반을 갖춘 상태입니다.

### 이미 완료한 것

- 기본 서버 부트스트랩
- 비동기 accept loop
- 클라이언트 세션 객체 생성
- 줄 단위 비동기 read loop
- 접속 / 종료 로그 출력
- `bad_weak_ptr` 원인 분석
- 해결 방향 확인

### 아직 하지 않은 것

- 세션 목록 관리
- 모든 클라이언트 대상 브로드캐스트
- 닉네임 등록
- 프로토콜 파싱
- 시스템 메시지 전파
- write queue
- WPF 클라이언트 구현

---

## 10. 다음 단계 권장 순서

다음 구현 순서는 아래처럼 진행하는 것이 좋습니다.

### Step 1. `ClientSession` 상속 수정 반영

- `public std::enable_shared_from_this<ClientSession>`로 수정
- 접속 및 read loop가 `bad_weak_ptr` 없이 정상 동작하는지 확인

### Step 2. `ChatServer`에 세션 목록 추가

- 현재 접속 중인 세션을 컨테이너로 관리
- join / remove 로직 추가
- 브로드캐스트를 위한 기반 마련

### Step 3. `ClientSession`에 write 기능 추가

- `Send()` 구현
- async write 충돌 방지를 위한 write queue 도입

### Step 4. 기본 프로토콜 파싱 추가

- `NICK|...` 파싱
- `MSG|...` 파싱
- `SYSTEM|...`, `CHAT|...` 메시지 생성

### Step 5. 브로드캐스트 로직 구현

- 한 클라이언트가 보낸 메시지를 모든 접속 세션에 전달

### Step 6. 임시 콘솔 테스트 클라이언트 또는 TCP 도구 활용

- WPF 이전에 프로토콜이 정상 동작하는지 검증

### Step 7. WPF 클라이언트 구현 시작

- 연결 UI
- 채팅 로그 UI
- 메시지 입력 UI
- 네트워크 서비스 레이어
- MVVM 구조 적용

---

## 11. 추천 마일스톤 계획

### Milestone 1
- 서버 시작
- async accept
- 줄 단위 read
- disconnect 처리

### Milestone 2
- 세션 컨테이너
- 브로드캐스트
- 닉네임 처리
- 시스템 메시지

### Milestone 3
- 콘솔 기반 프로토콜 검증

### Milestone 4
- WPF 클라이언트 MVP

### Milestone 5
- 리팩터링 및 확장 기능

---

## 12. 추천 커밋 메시지

```text
feat: add minimal Boost.Asio TCP server skeleton
feat: implement async accept and line-based read loop
fix: resolve bad_weak_ptr by using public enable_shared_from_this inheritance
feat: add session container for connected clients
feat: implement broadcast messaging
feat: add nickname registration protocol
feat: create WPF chat client MVP
refactor: separate network service from WPF UI logic
```

---

## 13. 한 줄 요약

이 프로젝트는 **Boost.Asio 기반 채팅 서버 + WPF 데스크톱 클라이언트** 구조로 기획되었습니다.

현재까지는 서버가 다음을 수행할 수 있는 단계까지 구현되었습니다.

- 포트 열기
- 클라이언트 접속 수락
- 줄 단위 비동기 메시지 수신
- 연결 종료 감지

개발 과정에서 발생한 핵심 문제는 `enable_shared_from_this`의 private 상속으로 인한 `bad_weak_ptr`였고, 올바른 해결 방법은 이를 public 상속으로 변경하는 것입니다.

다음 목표는 세션 관리, 브로드캐스트, 프로토콜 파싱을 추가해 실제 채팅 서버 형태로 확장한 뒤, 그 위에 WPF 클라이언트를 붙이는 것입니다.
