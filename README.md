# Boost.Asio + WPF Chat App

## 1. 프로젝트 소개

**Boost.Asio 기반의 TCP 채팅 서버**와 **WPF 기반의 데스크톱 채팅 클라이언트**를 구현하는 프로젝트입니다.  
이 프로젝트의 목표는 단순히 채팅 기능을 만드는 것에 그치지 않고, 다음과 같은 핵심 역량을 실제 코드로 보여주는 것입니다.

- C++ 기반 비동기 네트워크 서버 구현 능력
- 세션 관리 및 다중 클라이언트 처리 능력
- TCP 소켓 통신 구조에 대한 이해
- C# / WPF를 활용한 데스크톱 UI 애플리케이션 개발 능력
- 서버와 클라이언트 간 프로토콜 설계 능력
- 네트워크 로직과 UI 로직을 분리하는 구조 설계 능력

Windows 환경을 기준으로 개발하며, 서버는 **Boost.Asio**, 클라이언트는 **WPF(.NET)** 를 사용합니다.

---

## 2. 프로젝트 목표

이 프로젝트의 MVP(Minimum Viable Product) 목표는 다음과 같습니다.

- 클라이언트가 서버에 접속할 수 있다.
- 사용자가 닉네임을 등록할 수 있다.
- 사용자가 메시지를 입력하면 서버를 통해 전체 사용자에게 전송된다.
- 사용자의 입장 / 퇴장 이벤트가 다른 사용자에게 표시된다.
- WPF 클라이언트에서 채팅 내역과 연결 상태를 확인할 수 있다.

이후 포트폴리오 완성도를 높이기 위해 아래 기능들을 확장 목표로 둡니다.

- 접속자 목록 표시
- 귓속말 기능
- 채팅방 분리
- 서버 로그 저장
- JSON 기반 메시지 프로토콜 확장
- 재연결 처리
- 비정상 종료 대응

---

## 3. 기술 스택

### Server
- **Language**: C++17 이상
- **Network Library**: Boost.Asio
- **Build**: CMake
- **Environment**: Windows

### Client
- **Language**: C#
- **Framework**: .NET + WPF
- **Architecture**: MVVM

### Common
- **Protocol**: TCP 기반 텍스트 프로토콜 (초기) → JSON 프로토콜 (확장 가능)
- **Version Control**: Git / GitHub

---

## 4. 아키텍처 개요

```text
[WPF Client] -- TCP --> [Boost.Asio Chat Server] <-- TCP -- [WPF Client]
                                |
                                +-- Session Manager
                                +-- Client Session
                                +-- Broadcast Logic
```

## 향후 개선 방향
SSL/TLS 적용
DB 연동을 통한 채팅 기록 저장
로그인 기능 추가
방별 권한 관리
파일 전송 기능
하트비트 기반 연결 상태 점검
멀티스레드 io_context 실행 구조 확장