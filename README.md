# native-webrtc-manual-signaling
Manual signaling and simple loopback sample of native-webrtc 

this program is webrtc server and client sample.
client is simple manual signaling.
server is simple loopback and manual signaling.

see too qiita.


# install and run

1. install libwebrtc
1. ninja -C ./
1. ./sample
1. install and set up https server.
1. access to simple_p2p.html
1. click start button.
1. click create offer button.
1. copy offer and paste server.
1. server puts answer and offer.
1. copy "server answer" (not offer!) and paste to client.
1. click receive answer button.
1. copy "server offer" and paste to client.
1. click receive server offer button.
1. client puts answer.
1. copy and paste to server.
1. it work!

# program topic
message loop.

```c++
int main() {
   ... // webrtc set up.

  // this is point.
  (rtc::Thread::Current())->Run();
  
  return 0;
}
```
