#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/ssladapter.h"

class DummySetSessionDescriptionObserver
: public webrtc::SetSessionDescriptionObserver {
public:
   static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		LOG(INFO) << __FUNCTION__;
	}
	virtual void OnFailure(const std::string& error) {
		LOG(INFO) << __FUNCTION__ << " " << error;
	}

	protected:
	DummySetSessionDescriptionObserver() {}
	~DummySetSessionDescriptionObserver() {}
};

class PeerConnectionCallback : public webrtc::PeerConnectionObserver{
	private:
		std::function<void(rtc::scoped_refptr<webrtc::MediaStreamInterface>)> onAddStream;
		std::function<void(const webrtc::IceCandidateInterface*)> onIceCandidate;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
	public:
		PeerConnectionCallback() 
		: onAddStream(nullptr), onIceCandidate(nullptr) {
		}
		virtual ~PeerConnectionCallback() {}
    void SetOnAddStream(std::function<void(rtc::scoped_refptr<webrtc::MediaStreamInterface>)> addStream) {
      onAddStream = addStream;
    }
    void SetPeerConnection(rtc::scoped_refptr<webrtc::PeerConnectionInterface> p) {
      pc = p;
    }
protected:
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  }
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {
	  LOG(INFO) << __FUNCTION__ << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1";
  }
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  };
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  };
  void OnIceConnectionReceivingChange(bool receiving) override {
	  LOG(INFO) << __FUNCTION__ << " " << receiving;
  }

  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
    std::string msg;
    candidate->ToString(&msg);
	  LOG(INFO) << __FUNCTION__ << " " << msg;

    std::string sdp;
    pc->local_description()->ToString(&sdp);

        std::cout << "\n\n" << "3 -------------------- send server offer ---------------------" << std::endl;
				std::cout << sdp << std::endl;
        std::cout << "3 -------------------- send server offer ---------------------" << std::endl;
  };
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
	  LOG(INFO) << __FUNCTION__ << " " << stream->label();
  };
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
    LOG(INFO) << __FUNCTION__ << " " << stream->label();
    onAddStream(stream);
  };

};

class CreateSDPCallback : public webrtc::CreateSessionDescriptionObserver {
	private:
		std::function<void(webrtc::SessionDescriptionInterface*)> success;
		std::function<void(const std::string&)> failure;
	public:
		CreateSDPCallback(std::function<void(webrtc::SessionDescriptionInterface*)> s, std::function<void(const std::string&)> f)
      : success(s), failure(f) {
		};
		void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
      LOG(INFO) << __FUNCTION__ ;
			if (success) {
				success(desc);
			}
		}
		void OnFailure(const std::string& error) {
      LOG(INFO) << __FUNCTION__ ;
			if (failure) {
				failure(error);
			} else {
				LOG(LERROR) << error;
			}
		}
};

rtc::scoped_refptr<webrtc::PeerConnectionInterface> CreatePeerConnection(webrtc::PeerConnectionObserver* observer) {
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory
    = webrtc::CreatePeerConnectionFactory();
	if (!peer_connection_factory.get()) {
	  std::cout << "Failed to initialize PeerConnectionFactory" << std::endl;
	  return nullptr;
	}

  webrtc::FakeConstraints constraints;
  constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;

	server.uri = "stun:xxx.xxx.xxx.xxx:3478";
	config.servers.push_back(server);

	server.uri = "turn:localhost:3478?transport=udp";
	//server.uri = "turn:xxx.xxx.xxx.xxx:3478?transport=udp";
	server.username = "";
	server.password = "";
	config.servers.push_back(server);

  return peer_connection_factory->CreatePeerConnection(
      config, &constraints, NULL, NULL, observer);
}

int main() {
  rtc::InitializeSSL();

  auto peer_connection_callback = new PeerConnectionCallback();
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection = CreatePeerConnection(peer_connection_callback);
  peer_connection_callback->SetPeerConnection(peer_connection);

	// receive offer
	{
		std::string sdp;
		{
      std::cout << "\n\n" << "1 -------------------- input browser offer ---------------------" << std::endl;
			std::string input;
			do {
				std::getline(std::cin, input);
				if (input != "") {
          sdp += input + '\n';
        }
			} while (input != "");
      std::cout << "1 -------------------- input browser offer ---------------------" << std::endl;
		}

    /*
		Json::Reader reader;
		Json::Value jmessage;
		if (!reader.parse(sdp, jmessage)) {
			LOG(WARNING) << "Received unknown message. " << sdp;
			return 1;
		}
    */

		webrtc::SdpParseError error;
		webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription("offer", sdp, &error));
		if (!session_description) {
		  LOG(WARNING) << "Can't parse received session description message. "
			  << "SdpParseError was: " << error.description;
		  return 1;
		}
		LOG(INFO) << " Received session description :" << sdp;

    peer_connection_callback->SetOnAddStream(
      [&](rtc::scoped_refptr<webrtc::MediaStreamInterface> stream){
        peer_connection->AddStream(stream);
    });

		LOG(INFO) << " set remote description start.";
		peer_connection->SetRemoteDescription( DummySetSessionDescriptionObserver::Create(), session_description);
		LOG(INFO) << " set remote description end.";
	}

	
	// send answer
  LOG(INFO) << " create answer start.";
	peer_connection->CreateAnswer(new rtc::RefCountedObject<CreateSDPCallback>(
		[&](webrtc::SessionDescriptionInterface* desc) {
      LOG(INFO) << " create answer callback start.";
		  peer_connection->SetLocalDescription( DummySetSessionDescriptionObserver::Create(), desc);

		  std::string sdp;
		  desc->ToString(&sdp);

		  Json::StyledWriter writer;
		  Json::Value jmessage;
		  jmessage["type"] = desc->type();
		  jmessage["sdp"] = sdp;
      std::cout << "\n\n" << "2 -------------------- send server answer ---------------------" << std::endl;
		  // std::cout << writer.write(jmessage) << std::endl;
		  std::cout << sdp << std::endl;
      std::cout << "2 -------------------- send server answer ---------------------" << std::endl;
		  
      LOG(INFO) << " create answer callback end.";

      // create offer
      LOG(INFO) << " create offer start.";
      peer_connection->CreateOffer(new rtc::RefCountedObject<CreateSDPCallback>(
      [&](webrtc::SessionDescriptionInterface* desc){
        LOG(INFO) << " create offer callback start.";
        peer_connection->SetLocalDescription( DummySetSessionDescriptionObserver::Create(), desc);

        /* 
         * DO NOT SEND OFFER! becasuse ice is not included and this program is vanilla!
        std::string sdp;
        desc->ToString(&sdp);

        Json::StyledWriter writer;
        Json::Value jmessage;
        jmessage["type"] = desc->type();
        jmessage["sdp"] = sdp;
        std::cout << "\n\n" << "3 -------------------- send server offer ---------------------" << std::endl;
        // std::cout << writer.write(jmessage) << std::endl;
        std::cout << sdp << std::endl;
        std::cout << "3 -------------------- send server offer ---------------------" << std::endl;
        */
        
        // receive answer
        {
          std::string sdp;
          {
            std::cout << "\n\n" << "4 -------------------- input browser answer ---------------------" << std::endl;
            std::string input;
            do {
              std::getline(std::cin, input);
              if (input != "") {
                sdp += input + '\n';
              }
            } while (input != "");
            std::cout << "4 -------------------- input browser answer ---------------------" << std::endl;
          }

          /*
          Json::Reader reader;
          Json::Value jmessage;
          if (!reader.parse(sdp, jmessage)) {
            LOG(WARNING) << "Received unknown message. " << sdp;
            return;
          }
          */

          webrtc::SdpParseError error;
          webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription("answer", sdp, &error));
          if (!session_description) {
            LOG(WARNING) << "Can't parse received session description message. "
              << "SdpParseError was: " << error.description;
            return;
          }
          LOG(INFO) << " Received session description :" << sdp;
          peer_connection->SetRemoteDescription( DummySetSessionDescriptionObserver::Create(), session_description);
        }
        LOG(INFO) << " create offer callback start.";
      },
      nullptr
      ), NULL);

		},
		nullptr
	), NULL);
	

  (rtc::Thread::Current())->Run();
  rtc::CleanupSSL();
	return 0;
};
