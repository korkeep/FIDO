# FIDO-Standalone
## Design of FIDO Authentication System for Visually Impaired
FIDO-Standalone 프로젝트는 공개키 기반 생체인증 프로토콜인 FIDO(Fast Identity Online)와 개인키 관리 모듈인 SE(Secure Element)를 이용해 시각장애인도 쉽게 사용할 수 있는 인증 시스템을 제안한다. 로그인 단계의 진입 장벽을 허무는 것을 시작으로 ‘배리어- 프리(Barrier-Free)’ 환경을 구축하기 위한 ICT 기술의 활용 방향성 제시를 목표로 한다.

## FIDO UAF
![FIDO](https://user-images.githubusercontent.com/20378368/105572123-8f912b80-5d98-11eb-9600-12c5b7ceb644.PNG)
| Content | Description |
| --- | --- |
| Client | ASM ↔ Browser/App의 Data 전달 |
| ASM | Client ↔ Authenticator 통신을 위한 플러그인 |
| Authenticator | 전자서명 생성, 개인키 및 생체 정보 저장 |
| Server | 전자서명 검증, 개인키 및 증명 정보 저장 |
| Metadata | Authenticator의 신뢰성 검증을 위한 Data |

## Architecture
![ARCH](https://user-images.githubusercontent.com/20378368/105572210-1d6d1680-5d99-11eb-9278-2f8332cca328.PNG)
| Content | Description |
| --- | --- |
| RPi | FIDO Client 역할, 부팅과 동시에 에이전트 파일이 자동 실행 |
| Tomcat | FIDO Server와 연동, Apache의 웹 애플리케이션 Server |
| Docker | FIDO Server는 Docker로 실행됨, 컨테이너 기반 Server 관리 툴 |
| Firebase | 인증 성공 시 푸시 알림 전송됨, FCM(Firebase Cloud Messaging) 이용 |
| Arduino | AM(Authenticator Module) 역할, SE와 각종 센서를 제어하는 마이크로컨트롤러 |
| SE | RoT 환경 제공, 개인키 및 생체 정보 저장 |
| Fingerprint | 지문 센서, 지문 정보를 디지털 데이터로 변환 |
| Button | 버튼 센서, 인터럽트로 Arduino에 신호 전달 |
| Buzzer | 알림 센서, 시각장애인을 위한 음성 안내 기능 |

## Configuration Steps
- **Step 1**: Arduino 시리얼 버퍼 크기 늘려주기
```
- \Arduino\hardware\arduino\avr\cores\arduino
- HardwareSerial.h 파일의 Buffer 크기를 512로 설정
```
- **Step 2**: Android Application token 설정
```
- Android Source/SmartGate/app/release 폴더로 이동
- app-release.apk 파일을 스마트폰에 옮겨 App 설치
- App 실행시 Toast Message로 출력되는 token 값을 복사
- Server Source의 UAF.tar 압축 해제
- FIDO/UAF/fidouaf/src/main/java/org/ebayopensource/fidouaf/res 폴더로 이동
- FidUafResource.java 파일의 /public/authResponse에 token값 옮기기
```
- **Step 3**: mvn 패키지를 이용해 core 파일 설치
```
- UAF.tar 압축 해제 경로로 이동
- cd ./fido-uaf-core
- mvn clean install 명령어 입력
```
- **Step 4**: Docker 이미지 생성
```
- docker-fidouafserver.tar 압축 해제 경로로 이동
- docker exec -it <container ID> /bin/sh
```
- **Step 5**: Firebase 인증서 가져오기
```
- Firebase Console 접속
- 클라우드 메시징 → 웹 구성 선택
- '키 쌍 가져오기'를 통해 json 형식의 키 쌍 생성
- /usr/local/tomcat/conf 파일에 json 파일 저장
```
- **Step 6**: mvn 패키지를 이용해 전체 파일 설치
```
- UAF.tar 압축 해제 경로로 이동
- cd ./fidouaf
- mvn clean install 명령어 입력
```
- **Step 7**: 생성된 파일 Docker로 복사
```
- cp -r ~/.m2/repository/org/ebayopensource/fidouaf/0.0.1-SNAPSHOT/fidouaf-0.0.1-SNAPSHOT.war ~/FIDO/docker-fidouafserver/tomcat/fidouaf.war
```
- **Step 8**: Docker 이미지 실행
```
- docker-compose down
- docker-compose build
- docker-compose up
```
## Demo Video
- [YouTube Link](https://www.youtube.com/watch?v=aOKBzFgywHA)  
![캡처](https://user-images.githubusercontent.com/20378368/105572323-cf0c4780-5d99-11eb-900e-824e0e870d30.PNG)
