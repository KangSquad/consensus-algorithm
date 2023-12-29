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
#include "network-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

// 합의 알고리즘 변경 시 수정
//#include "../model/raft-node.h"
// #include "../model/paxos-node.h"
//#include "../model/pbft-node.h"
//#include "../model/pos-node.h"
#include "../model/pow.h"

namespace ns3 {

    NetworkHelper::NetworkHelper(uint32_t totalNoNodes)                     // NetworkHelper의 생성자
    {                   
        m_factory.SetTypeId ("ns3::PoW");                                   // 노드의 ID를 지정         // 합의 알고리즘 변경 시 수정
        m_nodeNo = totalNoNodes;                                            // 노드 전체 개수
    }
    
    ApplicationContainer NetworkHelper::Install (NodeContainer c)           // 각 노드에 application을 설치 (실제 컴퓨터에서 실행되는 거처럼 표현하기 위한 메소드를 제공하는 것이 application 클래스)
    {
        ApplicationContainer apps;                                          // application은 각 노드별로 독립적으로 정의되기 때문에 모든 apps를 저장해주는 app 컨테이너

        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)    // 모든 노드에
        {
            Ptr<PoW> app = m_factory.Create<PoW> ();                        // 추상 객체인 m_factory로 PoW 어플리케이션 생성      // 합의 알고리즘 변경 시 수정

            uint32_t nodeId = (*i)->GetId();                                // i 번째 노드의 id값

            app -> m_id = nodeId;                                           // (*app).m_id = nodeId (노드 id 초기화)
            app -> numNodes = m_nodeNo;                                     // (*app).N = m_nodeNo (전체 노드의 개수 초기화)
            app -> m_peersAddresses = m_nodesConnectionsIps[nodeId];        // nodeID번째의 IP (다른 노드들의 IP 저장)


            app -> m_peersIndex = m_nodesIndex;


            (*i)->AddApplication (app);                                     // i번째 노드에 앱을 추가

            apps.Add (app);                                                 // 모든 앱을 저장해두는 컨테이너에 PoW 애플리케이션 추가
        }
        return apps;
    }

}
