> 이 프로젝트는 Boost.Asio 기반의 비동기 TCP 채팅 서버로, `ChatServer`가 전체 연결과 브로드캐스트를 관리하고 `ClientSession`이 각 클라이언트의 명령 처리와 송수신을 담당하는 구조로 설계되었습니다. 클라이언트는 줄 단위 텍스트 프로토콜(`NICK`, `MSG`, `LIST`)을 통해 서버와 통신하며, 서버는 세션 큐 기반 송신 구조를 통해 메시지를 순차적으로 안전하게 전달합니다.

---

# Architecture

## 1. 개요

이 서버는 **이벤트 기반 비동기 네트워크 서버**이다.  
핵심 아이디어는 다음과 같다.

-   서버 프로세스는 `io_context.run()`을 중심으로 동작한다.
    
-   새로운 클라이언트 접속은 `ChatServer`가 `async_accept`로 받는다.
    
-   접속이 성립되면 각 클라이언트마다 `ClientSession` 객체가 생성된다.
    
-   각 세션은 `async_read_until('\n')`로 한 줄 단위 명령을 수신한다.
    
-   수신한 명령은 `HandleLine()`에서 해석된다.
    
-   메시지 송신은 세션 내부 `writeQueue_`를 통해 순차적으로 처리된다.
    
-   전체 접속자 목록은 `ChatServer`의 `sessions_`가 관리한다.
    

즉, 이 프로그램은  
**“서버가 접속을 받고 → 세션이 명령을 읽고 → 서버가 전체 상태를 관리하며 → 각 세션에 다시 결과를 전송하는 구조”** 로 동작한다.

----------

## 2. 전체 구조도

```text
+----------------------+
|        main          |
|----------------------|
| - io_context 생성     |
| - work_guard 생성     |
| - ChatServer 생성     |
| - io_context.run()    |
+----------+-----------+
           |
           v
+----------------------+
|     ChatServer       |
|----------------------|
| - acceptor_          |
| - sessions_          |
| - sessionsMutex_     |
|----------------------|
| + StartAccept()      |
| + Join()             |
| + Leave()            |
| + BroadcastRaw()     |
| + BroadcastSystem()  |
| + BroadcastChat()    |
| + IsNicknameAvailable() |
| + GetNicknames()     |
+----------+-----------+
           |
           | creates / manages
           v
+----------------------+
|    ClientSession     |
|----------------------|
| - socket_            |
| - buffer_            |
| - server_            |
| - writeQueue_        |
| - nickname_          |
| - hasJoined_         |
| - isClosed_          |
| - writeQueueMutex_   |
| - closeMutex_        |
|----------------------|
| + Start()            |
| + Send()             |
| + DoRead()           |
| + DoWrite()          |
| + HandleLine()       |
| + Close()            |
| + GetNickname()      |
| + HasNickname()      |
+----------------------+

```

이 구조에서 중요한 점은 **책임 분리**다.

-   `ChatServer`  
    → **전체 서버 관점의 책임** 담당  
    (접속 수락, 세션 등록/해제, 브로드캐스트, 닉네임 중복 검사, 사용자 목록 조회)
    
-   `ClientSession`  
    → **개별 클라이언트 연결 관점의 책임** 담당  
    (입력 읽기, 명령 파싱, 출력 큐 관리, 연결 종료 처리)
    

이렇게 분리되어 있기 때문에,  
서버 전체 정책과 개별 연결 로직이 섞이지 않고 비교적 명확하게 나뉜다.

----------

# Component Details

## 3. `main`의 역할

`main`은 서버 실행의 시작점이다.

```cpp
boost::asio::io_context ioContext;
auto workGuard = boost::asio::make_work_guard(ioContext);
ChatServer chatServer(ioContext, port);
ioContext.run();

```

여기서 실행 흐름은 다음과 같다.

1.  `io_context`를 생성한다.
    
2.  `work_guard`를 생성해 이벤트 루프가 너무 빨리 종료되지 않도록 한다.
    
3.  `ChatServer`를 생성한다.
    
4.  `io_context.run()`을 호출해 비동기 이벤트 루프를 시작한다.
    

즉, `main`은 실질적인 채팅 로직을 처리하지 않는다.  
**이벤트 루프를 준비하고 서버 객체를 가동시키는 역할**만 맡는다.

----------

## 4. `ChatServer`의 역할

`ChatServer`는 서버 전체를 대표하는 객체다.  
핵심 멤버는 다음과 같다.

-   `acceptor_` : 새 TCP 연결을 받는 객체
    
-   `sessions_` : 현재 접속 중인 모든 `ClientSession` 저장
    
-   `sessionsMutex_` : `sessions_` 보호용 mutex
    

### 4.1 접속 수락: `StartAccept()`

`StartAccept()`는 `acceptor_.async_accept(...)`를 호출해 비동기적으로 새 연결을 기다린다.  
연결이 들어오면 다음 순서로 처리한다.

1.  클라이언트의 IP/Port 로그 출력
    
2.  `ClientSession` 생성
    
3.  `Join(session)`으로 세션 등록
    
4.  `session->Start()` 호출
    
5.  마지막에 다시 `StartAccept()` 호출
    

즉, **한 번 accept하고 끝나는 구조가 아니라, accept가 끝날 때마다 다시 다음 accept를 등록하는 재귀형 이벤트 루프**다. 이 때문에 서버는 지속적으로 새로운 클라이언트를 받을 수 있다.

### 4.2 세션 등록/해제: `Join()`, `Leave()`

-   `Join()`  
    세션을 `sessions_`에 추가한다.
    
-   `Leave()`  
    세션을 `sessions_`에서 제거한다.
    

두 함수 모두 `sessionsMutex_`로 보호된다.  
즉, 접속/종료가 여러 타이밍에서 발생하더라도 세션 컨테이너를 안전하게 갱신하려는 의도가 보인다.

### 4.3 브로드캐스트: `BroadcastRaw()`, `BroadcastSystem()`, `BroadcastChat()`

브로드캐스트 계층도 잘 나뉘어 있다.

-   `BroadcastRaw()`  
    실제 전송 담당. `sessions_`를 순회해서 모든 세션의 `Send()`를 호출한다.
    
-   `BroadcastSystem()`  
    `"SYSTEM|..."` 형식 패킷을 만들어 `BroadcastRaw()`에 넘긴다.
    
-   `BroadcastChat()`  
    `"CHAT|nickname|message"` 형식 패킷을 만들어 `BroadcastRaw()`에 넘긴다.
    

여기서 좋은 점은,  
`BroadcastRaw()`가 직접 `sessions_`를 오래 잡고 순회하지 않도록 **먼저 로컬 `vector`에 세션 복사본을 만든 뒤 락을 풀고 전송**한다는 점이다.  
즉, 브로드캐스트 중에 세션 목록 락을 길게 점유하지 않도록 설계되어 있다.

### 4.4 닉네임 검사 및 목록 조회

-   `IsNicknameAvailable()`  
    요청한 닉네임이 이미 다른 세션에서 사용 중인지 검사한다.
    
-   `GetNicknames()`  
    현재 닉네임이 등록된 세션들의 이름만 모아서 반환한다.
    

이 함수들은 채팅 프로토콜에서 `NICK`, `LIST` 명령을 처리할 때 사용된다.

----------

## 5. `ClientSession`의 역할

`ClientSession`은 **클라이언트 1명과 서버 사이의 연결 상태 전체**를 표현한다.  
핵심 멤버는 다음과 같다.

-   `socket_` : 해당 클라이언트와 연결된 소켓
    
-   `buffer_` : 수신 버퍼
    
-   `server_` : 소속 서버 포인터
    
-   `writeQueue_` : 송신 대기열
    
-   `nickname_` : 사용자 닉네임
    
-   `hasJoined_` : 실제 입장 상태 여부
    
-   `isClosed_` : 중복 종료 방지 플래그
    
-   `writeQueueMutex_`, `closeMutex_` : 쓰기 큐/종료 보호용 mutex
    

또한 `std::enable_shared_from_this<ClientSession>`를 상속한다.  
이건 비동기 콜백이 실행되는 동안 세션 객체가 살아있도록 하기 위한 핵심 장치다.  
`DoRead()`, `DoWrite()`, `HandleLine()`에서 `auto self = shared_from_this();`를 사용하는 이유가 바로 이것이다.

----------

# Execution Flow

## 6. 프로그램 실행 흐름

아래는 프로그램 전체 수명주기를 기준으로 본 실행 흐름이다.

### 6.1 서버 시작

```text
main()
 └─ io_context 생성
 └─ work_guard 생성
 └─ ChatServer 생성
     └─ StartAccept() 등록
 └─ io_context.run()

```

이 시점부터 서버는 이벤트를 기다리는 상태가 된다.

----------

## 6.2 클라이언트 접속

```text
클라이언트 TCP 연결 시도
 └─ ChatServer::StartAccept() 콜백 실행
     └─ ClientSession 생성
     └─ ChatServer::Join(session)
     └─ session->Start()
         └─ ClientSession::DoRead()
     └─ 다음 접속을 위해 StartAccept() 재등록

```

즉, 접속 직후 세션 객체가 만들어지고, 곧바로 해당 세션은 **비동기 수신 대기 상태**로 들어간다.

----------

## 6.3 클라이언트 명령 수신

`ClientSession::DoRead()`는 `async_read_until(socket_, buffer_, '\n', ...)`를 사용한다.  
즉, 이 서버는 **줄 단위 프로토콜(line-based protocol)** 을 사용한다.

처리 순서는 다음과 같다.

1.  소켓에서 `'\n'`이 나올 때까지 비동기 수신
    
2.  수신이 완료되면 `buffer_`에서 한 줄 추출
    
3.  끝에 `'\r'`이 남아 있으면 제거
    
4.  로그 출력
    
5.  `HandleLine(line)` 호출
    
6.  다시 `DoRead()` 호출하여 다음 입력 대기
    

이 구조 때문에 서버는  
**명령 1개를 처리한 뒤 종료되는 방식이 아니라, 같은 연결에서 여러 줄 명령을 계속 처리**할 수 있다.

----------

## 7. 명령 처리 흐름: `HandleLine()`

이 함수가 사실상 이 서버의 **프로토콜 해석기**다.

현재 지원하는 명령은 다음 3개다.

-   `LIST`
    
-   `NICK|닉네임`
    
-   `MSG|메시지`
    

지원하지 않는 형식은 에러 메시지를 반환한다.

----------

## 8. `LIST` 명령 처리

`LIST`는 구분자 `|` 없이 단독 명령으로 처리된다.

```text
ClientSession::HandleLine("LIST")
 └─ ChatServer::GetNicknames()
 └─ "USERS|name1,name2,..." 형식 생성
 └─ 자기 자신에게 Send()

```

즉, 이 명령은 브로드캐스트가 아니라 **요청한 클라이언트에게만 현재 사용자 목록을 반환**한다.

예시 응답:

```text
USERS|Alice,Bob,Charlie

```

----------

## 9. `NICK|이름` 명령 처리

닉네임 등록 또는 변경 명령이다.

처리 흐름은 다음과 같다.

1.  payload가 비어 있으면 에러 반환
    
2.  `ChatServer::IsNicknameAvailable()`로 중복 검사
    
3.  이전 닉네임이 비어 있으면 **최초 등록**
    
4.  이전 닉네임이 있으면 **닉네임 변경**
    
5.  결과를 시스템 메시지로 전송
    

### 9.1 최초 등록인 경우

```text
NICK|Alice
 └─ nickname_가 비어 있음
 └─ hasJoined_ = true
 └─ 자기 자신에게 "SYSTEM|Nickname registered: Alice"
 └─ 전체에게 "SYSTEM|Alice joined" 브로드캐스트

```

### 9.2 변경인 경우

```text
NICK|Alice2
 └─ 기존 nickname_ 존재
 └─ oldNickname 저장
 └─ nickname_ 갱신
 └─ 자기 자신에게 "SYSTEM|Nickname change to: Alice2"
 └─ 전체에게 "SYSTEM|Alice changed nickname to Alice2" 브로드캐스트

```

즉, `NICK` 명령은 단순히 이름만 저장하는 것이 아니라,  
**최초 입장과 이름 변경이라는 두 상황을 구분하여 시스템 이벤트를 발생시키는 명령**이다.

----------

## 10. `MSG|내용` 명령 처리

일반 채팅 메시지 전송 명령이다.

처리 흐름은 다음과 같다.

1.  아직 닉네임이 없다면 거부  
    (`NICK|YourName` 먼저 요구)
    
2.  메시지가 비어 있으면 거부
    
3.  `ChatServer::BroadcastChat()` 호출
    
4.  서버가 `"CHAT|nickname|message"` 형태로 전체 클라이언트에게 브로드캐스트
    

즉, 메시지 흐름은 아래와 같다.

```text
클라이언트: MSG|Hello
 └─ ClientSession::HandleLine()
     └─ ChatServer::BroadcastChat(session, "Alice", "Hello")
         └─ BroadcastRaw()
             └─ 모든 세션의 Send("CHAT|Alice|Hello\n")

```

현재 구현상 `BroadcastRaw()`는 모든 세션에게 전송하므로,  
**보낸 사람 자신도 브로드캐스트 결과를 다시 수신**하게 된다.

----------

# Sending Logic

## 11. 송신 구조: 왜 `writeQueue_`가 필요한가

비동기 서버에서 소켓에 대한 쓰기는 매우 중요하다.  
여러 메시지를 동시에 `async_write` 해버리면 전송 순서가 꼬이거나 충돌할 수 있다.  
그래서 이 코드는 세션마다 `writeQueue_`를 둔다.

### 11.1 `Send()`의 동작

`Send()`는 바로 쓰지 않고 먼저 큐에 넣는다.

```text
Send(message)
 └─ writeQueueMutex_ lock
 └─ 현재 쓰기 진행 중인지 확인
 └─ queue에 push
 └─ 이전에 쓰기 중이 아니었다면 DoWrite() 시작

```

즉, 첫 메시지만 실제 쓰기를 시작하고,  
그 뒤 메시지들은 큐에 쌓인다.

### 11.2 `DoWrite()`의 동작

`DoWrite()`는 큐의 맨 앞 메시지를 `async_write`로 보낸다.  
전송이 끝나면:

1.  큐 front 제거
    
2.  아직 큐에 남은 메시지가 있으면 다시 `DoWrite()` 호출
    

이 구조 덕분에 같은 세션의 송신은 항상 **한 번에 하나씩 순차 처리**된다.  
즉, `writeQueue_`는 단순 버퍼가 아니라 **비동기 송신 직렬화 장치**다.

----------

# Connection Lifecycle

## 12. 연결 종료 흐름: `Close()`

`Close()`는 세션 종료를 담당한다.  
이 함수는 여러 에러 경로에서 동시에 호출될 수 있기 때문에 `closeMutex_`와 `isClosed_`로 중복 종료를 막는다.

처리 순서는 다음과 같다.

1.  이미 닫혔으면 바로 return
    
2.  `isClosed_ = true`
    
3.  닉네임 등록 후 입장한 상태였다면 `"nickname left"` 시스템 메시지 브로드캐스트
    
4.  소켓 `shutdown`
    
5.  소켓 `close`
    
6.  서버에 `Leave(shared_from_this())` 요청
    

즉, 종료 시점에도 단순히 소켓만 닫는 것이 아니라:

-   사용자 퇴장 이벤트를 알리고
    
-   세션 목록에서 제거하고
    
-   리소스를 정리하는
    

**정리 중심의 종료 처리**를 수행한다.

----------

## 13. 에러 발생 시 흐름

### 13.1 읽기 에러

`DoRead()` 콜백에서 에러가 나면:

-   EOF면 정상 종료 로그
    
-   그 외면 에러 코드/카테고리/메시지 출력
    
-   이후 `Close()` 호출
    

### 13.2 쓰기 에러

`DoWrite()` 콜백에서 에러가 나면:

-   에러 메시지 출력
    
-   `Close()` 호출
    

### 13.3 accept 에러

`StartAccept()`에서 accept 실패 시:

-   accept 에러 로그 출력
    
-   그래도 마지막에 다시 `StartAccept()` 호출
    

즉, 이 서버는 일부 에러가 발생해도  
**서버 전체를 멈추기보다 해당 연결만 정리하고 다음 이벤트를 계속 받는 구조**를 지향한다.

----------

# Protocol

## 14. 프로토콜 규약

이 서버는 **텍스트 기반 라인 프로토콜**을 사용한다.  
각 명령은 `\n`으로 끝나야 하며, 서버는 `async_read_until('\n')`로 입력을 구분한다. `\r\n`이 들어와도 끝의 `\r`을 제거하도록 처리되어 있다.

### 클라이언트 → 서버

```text
NICK|Alice
MSG|Hello everyone
LIST

```

### 서버 → 클라이언트

```text
SYSTEM|Nickname registered: Alice
SYSTEM|Alice joined
SYSTEM|Alice changed nickname to Alice2
SYSTEM|Nickname already in use
SYSTEM|Register nickname first using "NICK|YourName"

CHAT|Alice|Hello everyone
USERS|Alice,Bob,Charlie

```

----------

# Why This Design Works

## 15. 이 구조의 장점

### 15.1 책임이 분리되어 있다

-   `ChatServer`는 전체 상태 관리
    
-   `ClientSession`은 연결 1개 처리
    

덕분에 코드를 읽을 때  
“이 로직이 서버 전체 정책인지, 개별 연결 처리인지”가 구분된다.

### 15.2 비동기 기반이라 연결 수용 흐름이 자연스럽다

`async_accept`, `async_read_until`, `async_write`를 사용하므로  
각 클라이언트에 대해 블로킹 없이 이벤트 중심으로 동작한다.

### 15.3 송신 순서를 세션별 큐로 보장한다

`writeQueue_` 덕분에 한 세션에 대한 여러 메시지가 순서대로 안전하게 전송된다.

### 15.4 세션 수명 관리가 비교적 안전하다

`shared_from_this()`를 사용해 비동기 콜백 동안 객체 생존을 유지한다.

### 15.5 종료 처리가 명시적이다

읽기/쓰기 에러가 나면 `Close()`로 수렴하고, 그 안에서 퇴장 브로드캐스트와 세션 제거가 한 번만 실행되도록 되어 있다.

----------

# End-to-End Example

## 16. 실제 동작 예시

### 상황

사용자 `Alice`와 `Bob`이 접속해 채팅하는 상황

### 흐름

```text
1. Alice 접속
   -> ChatServer가 accept
   -> ClientSession 생성
   -> sessions_에 등록
   -> DoRead() 시작

2. Alice가 "NICK|Alice" 전송
   -> HandleLine()
   -> 닉네임 중복 검사
   -> nickname_ = "Alice"
   -> Alice에게 "SYSTEM|Nickname registered: Alice"
   -> 전체에게 "SYSTEM|Alice joined"

3. Bob 접속
   -> 같은 과정 반복

4. Bob이 "NICK|Bob" 전송
   -> Bob 등록
   -> 전체에게 "SYSTEM|Bob joined"

5. Alice가 "MSG|Hello" 전송
   -> HandleLine()
   -> BroadcastChat("Alice", "Hello")
   -> 모든 세션에게 "CHAT|Alice|Hello"

6. Bob이 "LIST" 전송
   -> GetNicknames()
   -> Bob에게만 "USERS|Alice,Bob"

7. Bob 연결 종료
   -> DoRead() 또는 DoWrite()에서 에러/EOF 감지
   -> Close()
   -> 전체에게 "SYSTEM|Bob left"
   -> sessions_에서 제거

```

이 예시를 통해 보면 이 서버는  
**입장 / 채팅 / 목록 조회 / 퇴장**이라는 채팅 서버의 기본 흐름을 이벤트 기반으로 자연스럽게 처리하고 있다.