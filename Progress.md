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
- 닉네임 중복 검사
- 접속자 목록 조회 명령

---

## 3. 개발 전략

추천 구현 순서는 다음과 같습니다.

1. **Boost.Asio 서버 골격부터 먼저 구현**
2. **간단한 테스트 클라이언트나 콘솔 TCP 도구로 통신 검증**
3. **세션 관리와 브로드캐스트 로직 추가**
4. **닉네임 및 메시지 프로토콜 처리 추가**
5. **그 다음에 WPF 클라이언트 구현**
6. **구조 리팩터링 및 확장 기능 추가**

이 순서가 중요한 이유는, WPF를 너무 일찍 만들기 시작하면 프로토콜 변경이 발생할 때 UI 작업을 반복하게 될 가능성이 크기 때문입니다.

---

## 4. 초기 프로토콜 기획

초기 단계에서는 구현 복잡도를 낮추기 위해 **줄 단위 텍스트 프로토콜**을 사용합니다.

### 클라이언트 → 서버

```text
NICK|UserA
MSG|안녕하세요
```

### 현재 서버가 전송하는 형식

현재 구현 기준으로는 다음과 같은 문자열 형식을 사용하고 있습니다.

```text
SYSTEM | UserA joined
UserA: 안녕하세요
```

즉, 초기 기획에서는 `SYSTEM|...`, `CHAT|nickname|message` 형식을 목표로 했지만, 현재 실제 구현은 **시스템 메시지용 문자열 형식**과 **일반 채팅 문자열 형식**을 사용하고 있습니다.

### 계획된 메시지 타입

- `NICK|{nickname}`
- `MSG|{message}`
- `SYSTEM|{message}`
- `CHAT|{nickname}|{message}`

향후 WPF 클라이언트 구현 전에는 서버 송신 형식을 다시 한 번 정규화하는 작업이 필요합니다.

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
- 입력 라인 끝의 `\r`을 제거해 콘솔/텔넷 입력에 대응한다.
- 연결 종료 또는 에러가 발생할 때까지 read loop를 계속 이어간다.

### 6.5 세션 목록 관리 구조 구현

`ChatServer`는 현재 **접속 중인 세션 목록**을 관리하고 있습니다.

구현된 핵심 내용은 다음과 같습니다.

- `sessions_` 컨테이너로 접속 세션 보관
- `Join()`으로 세션 추가
- `Leave()`로 세션 제거
- 현재 접속자 수를 로그로 확인
- `sessionsMutex_`를 사용해 세션 컨테이너 접근 보호

이 구조를 통해 서버는 단순 수신 서버가 아니라 **다중 사용자 채팅 서버**의 기본 형태를 갖추게 되었습니다.

### 6.6 송신 기능 및 write queue 구현

기존에는 읽기만 가능했지만, 이제 각 세션이 서버가 전달하는 메시지를 다시 클라이언트에 보낼 수 있도록 `Send()` 구조가 구현되었습니다.

구현된 핵심 내용은 다음과 같습니다.

- `ClientSession::Send()` 구현
- `async_write` 기반 비동기 송신 처리
- 동시에 여러 write가 겹치는 문제를 줄이기 위한 `writeQueue_` 도입
- queue가 비어 있을 때만 실제 `DoWrite()`를 시작하는 구조
- `writeQueueMutex_`를 사용해 송신 큐 접근 보호

이 구조는 브로드캐스트, 시스템 메시지, 채팅 메시지를 안정적으로 보낼 수 있게 해주는 핵심 기반입니다.

### 6.7 브로드캐스트 구조 구현

서버는 여러 세션에 같은 메시지를 전달하기 위한 브로드캐스트 구조를 갖추고 있습니다.

현재 동작 흐름은 다음과 같습니다.

1. 어떤 클라이언트가 한 줄 메시지를 전송한다.
2. 해당 `ClientSession`이 메시지를 수신한다.
3. `HandleLine()`에서 명령을 해석한다.
4. 서버가 현재 접속 중인 세션들에 대해 `Send()`를 호출한다.
5. 각 세션이 자신의 write queue를 통해 메시지를 클라이언트에게 전송한다.

또한 현재 구현에서는 **메시지를 보낸 본인 세션은 브로드캐스트 대상에서 제외**하도록 처리되어 있습니다.

### 6.8 닉네임 등록 및 메시지 처리 구현

현재 서버는 기본적인 명령 프로토콜을 실제로 처리할 수 있습니다.

구현된 명령은 다음과 같습니다.

- `NICK|{nickname}`
- `MSG|{message}`

세부 동작은 다음과 같습니다.

#### `NICK|...`
- 닉네임이 비어 있으면 거부
- 최초 등록이면 `nickname_` 저장
- 등록 성공 시 본인에게 등록 완료 메시지 전송
- 다른 클라이언트들에게 입장 시스템 메시지 브로드캐스트
- 이미 닉네임이 있는 상태라면 닉네임 변경 메시지를 본인에게 전송

#### `MSG|...`
- 닉네임이 아직 없으면 메시지 전송 거부
- 메시지가 비어 있으면 거부
- 정상 메시지면 서버가 다른 세션들에게 브로드캐스트

즉, 현재는 단순 문자열 송수신을 넘어 **기본 채팅 명령을 해석하는 서버** 단계까지 도달한 상태입니다.

### 6.9 연결 종료 처리 구조 개선

클라이언트 연결 종료 시 세션이 서버의 세션 목록에서 빠지도록 `Close()` 흐름이 정리되어 있습니다.

핵심 포인트는 다음과 같습니다.

- read error 또는 EOF 발생 시 세션 종료
- 소켓 shutdown / close 처리
- 서버의 세션 목록에서 해당 세션 제거
- `isClosed_` 플래그로 중복 종료 방지
- `closeMutex_`로 close 처리 보호

이 구조가 있어야 접속 종료 후에도 서버 내부에 유효하지 않은 세션이 남지 않습니다.

### 6.10 현재까지의 도달 수준

현재 단계에서 다음 수준까지 도달했습니다.

- 서버 실행 가능
- 클라이언트 접속 수락 가능
- 클라이언트가 보낸 한 줄 텍스트 수신 가능
- 연결 종료 및 read error 감지 가능
- 세션 목록 관리 구현 완료
- `Send()` 및 write queue 구현 완료
- 브로드캐스트 구조 구현 완료
- `NICK|...`, `MSG|...` 명령 처리 구현 완료
- 입장 시스템 메시지 전송 구현 완료
- 발신자 제외 브로드캐스트 구현 완료

즉, 현재는 **기본 명령 처리와 다중 사용자 메시지 전파가 가능한 채팅 서버 1차 버전**까지 도달한 상태입니다.

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
- 브로드캐스트를 위해서는 `ClientSession`이 서버를 참조할 수 있어야 한다.
- 세션별 송신은 queue 기반으로 처리해야 한다.
- close는 중복 호출될 수 있으므로 보호 장치가 필요하다.
- 향후 WPF 클라이언트 구현 전에는 서버 송신 패킷 형식을 명확히 통일할 필요가 있다.

---

## 9. 현재 상태 요약

현재 구현은 아직 **완전한 채팅 서버**는 아니지만, 충분히 좋은 기반을 갖춘 상태입니다.

### 이미 완료한 것

- 기본 서버 부트스트랩
- 비동기 accept loop
- 클라이언트 세션 객체 생성
- 줄 단위 비동기 read loop
- 접속 / 종료 로그 출력
- `bad_weak_ptr` 원인 분석 및 해결
- 세션 목록 관리 구현
- write queue 기반 송신 구현
- 브로드캐스트 구현
- `NICK|...`, `MSG|...` 프로토콜 처리 구현
- 발신자 제외 브로드캐스트 구현
- close 중복 방지 구조 구현

### 아직 하지 않은 것

- 닉네임 중복 검사
- 퇴장 시스템 메시지 브로드캐스트
- 서버 송신 패킷 형식 통일
- 접속자 목록 기능
- WPF 클라이언트 구현

---

## 10. 다음 단계 권장 순서

다음 구현 순서는 아래처럼 진행하는 것이 좋습니다.

### Step 1. 닉네임 중복 검사 추가

- 동일 닉네임 사용 가능 여부 검사
- 중복일 경우 등록 거부
- 닉네임 변경 시에도 같은 규칙 적용

### Step 2. 서버 송신 패킷 형식 통일

- `SYSTEM|...`
- `CHAT|nickname|message`

이처럼 송신 형식을 정리해 WPF 클라이언트가 쉽게 파싱할 수 있도록 만든다.

### Step 3. 퇴장 시스템 메시지 추가

- 닉네임 등록을 마친 사용자가 종료되면 `left` 메시지 브로드캐스트

### Step 4. 접속자 목록 기능 추가

- `LIST` 명령 설계
- 현재 접속자 목록 반환
- WPF 사용자 목록 UI와 연결하기 쉽게 구조화

### Step 5. WPF 클라이언트 MVP 시작

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
- write queue
- 브로드캐스트
- 다중 클라이언트 통신 확인

### Milestone 3
- 닉네임 처리
- 메시지 처리
- 기본 채팅 흐름 완성

### Milestone 4
- 닉네임 중복 검사
- 시스템 메시지 정리
- 송신 패킷 정규화

### Milestone 5
- 접속자 목록 기능
- 콘솔 기반 프로토콜 검증 고도화

### Milestone 6
- WPF 클라이언트 MVP

### Milestone 7
- 리팩터링 및 확장 기능

---

## 12. 추천 커밋 메시지

```text
feat: add minimal Boost.Asio TCP server skeleton
feat: implement async accept and line-based read loop
fix: resolve bad_weak_ptr by using public enable_shared_from_this inheritance
feat: add session container for connected clients
feat: add write queue to client session
feat: implement broadcast messaging
feat: add nickname registration and message handling
feat: exclude sender from broadcast delivery
refactor: protect close and write queue with mutex
feat: create WPF chat client MVP
```

---

## 13. 한 줄 요약

이 프로젝트는 **Boost.Asio 기반 채팅 서버 + WPF 데스크톱 클라이언트** 구조로 기획되었습니다.

현재까지는 서버가 다음을 수행할 수 있는 단계까지 구현 및 정리되었습니다.

- 포트 열기
- 클라이언트 접속 수락
- 줄 단위 비동기 메시지 수신
- 연결 종료 감지
- 세션 목록 관리
- write queue 기반 송신
- 브로드캐스트
- 닉네임 등록 및 메시지 처리
- 발신자 제외 브로드캐스트
- close 중복 방지

다음 목표는 닉네임 중복 검사, 퇴장 시스템 메시지, 송신 패킷 형식 통일, 접속자 목록 기능을 추가해 서버 프로토콜을 더 안정적으로 정리한 뒤, 그 위에 WPF 클라이언트를 붙이는 것입니다.
