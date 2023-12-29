/*
Copyright (c) 2022 zhayujie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

The code was referenced from https://github.com/zhayujie/blockchain-simulator.
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;	//이렇게 함으로써 ns3:: 이거 안 써도 됨

NS_LOG_COMPONENT_DEFINE("PoWSimulator");

void startSimulator(int numNodes)
{
	NodeContainer nodes;
	nodes.Create(numNodes);																	// N개의 노드 생성

	
	PointToPointHelper pointToPoint;														// pointTopoint는 두 개의 노드끼리만 연결 가능하므로 하나씩 직접 연결해야함 (노드끼리의 채널)
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("3Mbps")); 						// 초당 3mb (통신을 위한 하드웨어 장치)
	pointToPoint.SetChannelAttribute("Delay", StringValue("3ms"));   						// 3밀리초를 전파 지연 값으로 사용

	InternetStackHelper stack;
	stack.Install(nodes);																	// 노드에 stack 설치 (TCP, UDP, IP 등과 같은 기능을 노드가 사용할 수 있게)


	Ipv4AddressHelper address;																// IP 할당 10.1.1.0 -> 10.1.1.1 -> 10.1.1.2 …
	address.SetBase("10.1.1.0", "255.255.255.0");											// IP 주소와, 서브넷 마스크
	// address.SetBase("192.168.1.0", "255.255.255.0");											// IP 주소와, 서브넷 마스크


	NetworkHelper networkHelper(numNodes);													// network-helper.h에 정의되어 있음


	for (int i=0; i < numNodes; i++)														// pointTopoint는 두 개의 노드끼리만 연결 가능하므로 직접 쌍으로 연결
	{

		for (int j=0; j<numNodes && j != i; j++)											// 자신을 제외한 다른 노드들과
		{											

			Ptr<Node> p1 = nodes.Get(i);
			Ptr<Node> p2 = nodes.Get(j);
			NetDeviceContainer device = pointToPoint.Install(p1, p2);						// 다른 노드와 통신할 수 있도록 하는 장치 (실제 컴퓨터의 NIC를 소프트웨어적으로 구현한 것) (채널)


			// NS_LOG_UNCOND("i : "<<i<< "   j : "<<j);

			// NS_LOG_UNCOND(device.Get(0)->GetAddress());									// MAC 주소
			// NS_LOG_UNCOND(device.Get(1)->GetAddress());
			Ipv4InterfaceContainer interface;												// IP와 Index (나중에 참조하기 위해)
			interface.Add(address.Assign(device.Get(0)));									// IP를 NetDevice에 Assign   	// (p1의 NetDevice)			// Assign의 return 값 : Ipv4InterfaceContainer
			interface.Add(address.Assign(device.Get(1)));									// interface.Add로 IP컨테이너에 IP 저장  // (p2의 NetDevice)
			
			// NS_LOG_UNCOND(nodes.Get(i)->GetId());										// 공개 ip주소
			// NS_LOG_UNCOND(interface.GetAddress(0));										// 공개 ip주소
			// NS_LOG_UNCOND(interface.GetAddress(1));
			// NS_LOG_UNCOND("");



			networkHelper.m_nodesConnectionsIps[i].push_back(interface.GetAddress(1));		// p1에게는 p2의 ip를 저장	(ip 저장해서 App에서 사용할 수 있도록 (PoW-Node.cc))
			networkHelper.m_nodesConnectionsIps[j].push_back(interface.GetAddress(0));		// p2에게는 p1의 ip를 저장
            
            
            networkHelper.m_nodesIndex[interface.GetAddress(0)] = i;

			address.NewNetwork();															// 네트워크 수 + 1, 네트워크 번호 +1  (10.1.1.0 -> 10.1.1.1 -> 10.1.1.2)
		}


        

	}

	ApplicationContainer nodeApp = networkHelper.Install (nodes);							// 노드에 App 설치

	nodeApp.Start (Seconds (0.0));															// 앱 실행
	nodeApp.Stop  (Seconds (10.0));															// 앱 종료
	
	Simulator::Run ();																		// 앱에서 스케줄한 시뮬레이터 실행
	Simulator::Destroy ();																	// 앱에서 스케줄한 시뮬레이터 종료
}

int main(int argc, char *argv[])
{

	int numNodes;																			// 노드의 개수

	CommandLine cmd;																		// argc : 메인 함수에 전달되는 정보의 개수
	cmd.AddValue("numNodes", "Number of node", numNodes=4);									// 명령어를 통해 노드의 개수 설정
	cmd.Parse(argc, argv);																	// argv : 메인 함수에 전달되는 정보로, 문자열의 배열 (첫 번째 문자열은 프로그램의 실행 경로가 디폴트)

	NS_LOG_UNCOND("Hello PoW Simulator" << endl);	

	Time::SetResolution(Time::NS); 															// Time을 표현하는 단위를 설정 (NanoSecond)(Default가 NanoSecond) 
	// NS_LOG(Time::GetDays());	

	LogComponentEnable("PoW", LOG_LEVEL_INFO);												// Log를 나타내겠다는 명령어 (Logging 활성화)
	startSimulator(numNodes);																// 함수 호출

	return 0;	

}
