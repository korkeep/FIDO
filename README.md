# FIDO-Standalone
FIDO-Standalone project proposes secure authentication system using biometric authentication protocol FIDO(Fast Identity Online) and key management module SE(Secure Element) ☝🔐  

## FIDO UAF
![FIDO](https://user-images.githubusercontent.com/20378368/105572123-8f912b80-5d98-11eb-9600-12c5b7ceb644.PNG)
| Content | Description |
| --- | --- |
| Client | Mediate data transfer between ASM and Application |
| ASM | Plug-in for FIDO communication between ASM and Client |
| Authenticator | Create digital signature, Store private key & bio info |
| Server | Verify digital signature, Store public key & auth info |
| Metadata | Metadata for authenticator verification |

## Architecture
![ARCH](https://user-images.githubusercontent.com/20378368/105572210-1d6d1680-5d99-11eb-9278-2f8332cca328.PNG)
| Content | Description |
| --- | --- |
| RPi | RPi acts as a FIDO Client. Upon booting, agent file is automatically executed. |
| Tomcat | Tomcat is Apache's web application server. It works as FIDO server. |
| Docker | Docker is container-based server management tool. FIDO Server runs through Docker. |
| Firebase | Provides FCM(Firebase Cloud Messaging) function. When authentication is successful, push notification is sent to Android device. |
| Arduino | Arduino is a microcontroller that controls SE and various sensors. It acts as Authenticator. |
| SE | SE ensures RoT(Root of Trust) environment. It stores private key and bio information. |
| Fingerprint | A sensor that converts fingerprint image into digital data. |
| Button | A sensor that transmits a signal to Arduino through interrupts. |

## Configuration Steps
- **Step 1**: Increase Arduino serial buffer size
```
$ cd {Arduino Install Path}\Arduino\hardware\arduino\avr\cores\arduino
# Set the buffer size of HardwareSerial.h file to 512
```
- **Step 2**: Config of Android Application Firebase token
```
$ cd {Path}/FIDO-Standalone/Android Source/SmartGate/app/release
# Install the app-release.apk file on the Android device
# Copy the token value displayed as Toast Message when running the Application
$ tar -xvf {Path}/FIDO-Standalone/Server Source/UAF.tar
$ cd {Path}/FIDO-Standalone/Server Source/UAF/fidouaf/src/main/java/org/ebayopensource/fidouaf/res
# Copy the token value displayed on the Android device to /public/authResponse in FidUafResource.java
```
- **Step 3**: Install core file using mvn package
```
$ cd {Path}/FIDO-Standalone/Server Source/UAF/fido-uaf-core
$ mvn clean install
```
- **Step 4**: Create Docker image
```
$ tar -xvf {Path}/FIDO-Standalone/Server Source/docker-fidouafserver.tar
$ cd {Path}/FIDO-Standalone/Server Source/docker-fidouafserver
$ docker exec -it {container ID} /bin/sh
```
- **Step 5**: Import Firebase Certificate
```
# Access the Firebase Console
# Cloud Messaging → Select Web Config
# Generating json format key pairs via 'Get Key Pair'
# Save json file in /usr/local/tomcat/conf
```
- **Step 6**: Entire file installation using mvn package
```
$ cd {Path}/FIDO-Standalone/Server Source/UAF/fidouaf
$ mvn clean install
```
- **Step 7**: Copy the generated file to Docker
```
$ cp -r ~/.m2/repository/org/ebayopensource/fidouaf/0.0.1-SNAPSHOT/fidouaf-0.0.1-SNAPSHOT.war ~/FIDO/docker-fidouafserver/tomcat/fidouaf.war
```
- **Step 8**: Run the Docker image
```
$ docker-compose down
$ docker-compose build
$ docker-compose up
```
## Demo Video
- [YouTube Link](https://www.youtube.com/watch?v=aOKBzFgywHA)  
> ![Video Capture](https://user-images.githubusercontent.com/20378368/105572323-cf0c4780-5d99-11eb-900e-824e0e870d30.PNG)
