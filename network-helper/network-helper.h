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



#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/uinteger.h"
#include <map>

namespace ns3 {

    class NetworkHelper
    {
        public:
            NetworkHelper (uint32_t totalNoNodes);                                      // 생성자
            std::map<uint32_t, std::vector<Ipv4Address>>   m_nodesConnectionsIps;       // 모든 노드들의 IP를 저장해주는 맵         // 각 노드의 id가 key이고 노드별로 ip를 가지고 있음
            std::map<Ipv4Address, uint32_t>   m_nodesIndex;                // ip주소가 어떤 노드껀지
            ApplicationContainer Install (NodeContainer c);                             // 노드에 어플리케이션 설치

        private:
            ObjectFactory               m_factory;                                      // 추상 객체로서 PoWNode 어플리케이션으로 초기화될 객체 (추상 객체)
            int                         m_nodeNo;                                       // 전체 노드의 개수
    };

}

#endif
