## FIDO Standalone 빌드법 정리

1. 기반 환경 세팅
- RPI Source : Authenticator ppt 참고
- Server Source : Authenticator ppt 참고
- Arduino Source : Authenticator ppt 참고
  Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino에서,
  HardwareSerial.h 파일의 Buffer 크기를 512로 늘려준다.
- Android Source : release 폴더의 apk파일을 스마트폰에 옮겨 App을 설치한다.
  App 실행 후, token을 복사해 FIDO Server의 “/public/authResponse”에 옮긴다.
  ~/FIDO/UAF/fidouaf/src/main/java/org/ebayopensource/fidouaf/res에서,
  FidUafResource.java파일 내부에서 수정할 수 있다.

2. cd ~/FIDO/UAF/fido-uaf-core → mvn clean install

3. Firebase SDK 환경변수 설정
- ~/FIDO/docker-fidouafserver 에서, sudo docker exec –it <container ID> /bin/sh
- /usr/local/tomcat/conf 파일 내부에 FCM json 파일 저장
- 상세 설명은 FIDO Standalone ppt 참고

4. cd ~/FIDO/UAF/fidouaf → mvn clean install

5. cp -r ~/.m2/repository/org/ebayopensource/fidouaf/0.0.1-SNAPSHOT/fidouaf-0.0.1-SNAPSHOT.war ~/FIDO/docker-fidouafserver/tomcat/fidouaf.war

6. ~/FIDO/docker-fidouafserver
 → sudo docker-compose down
 → sudo docker-compose build
 → sudo docker-compose up

7. cd /etc/xdg/lxsession/LXDE-pi/
 → vi autostart
 → lxterminal –e <파일 경로> 마지막 문단에 추가

8. C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino
 → HardwareSerial.h 파일 선택
 → SERIAL_TX_BUFFER_SIZE 512
 → SERIAL_RX_BUFFER_SIZE 512 로 변경 필요!