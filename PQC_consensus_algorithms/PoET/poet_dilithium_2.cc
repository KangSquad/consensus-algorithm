#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "poet_dilithium_2.h"
#include "stdlib.h"
#include "ns3/ipv4.h"
#include "SHA.h"
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <map>
#include <random>
#include <string>
#include <iomanip>


using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> dis (1, 99);

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("PoET");
    NS_OBJECT_ENSURE_REGISTERED(PoET);

    static Network network;

    Network::Network(void) {
        numNodes = 4;

        blockSize           = 20480;
        prevHashSize        = 64;
        txStorage           = blockSize - prevHashSize;

        minimumWait = 5;
        localAverage = 5 * numNodes;
        zMax = 2.325;                                       // 1.645, 2.325, 2.575, and 3.075

        winNum.assign(numNodes, 0);

        numTxs = 1000;
        maxTxItemSize = 512;                        // 256 bytes

    }

    Network::~Network(void) {
        // NS_LOG_FUNCTION(this);
    }

    // =============== 체인의 구성요소 출력 =============== //
    void
    Network::PrintChain(string chainName, Blockchain chain) {
        cout << endl;
        for(int i = 0; i < chain.GetBlockchainHeight(); i++) {
            string blockName = chainName + "'s [" + to_string(i) + "]th Block";
            PrintBlock(blockName, chain.blocks[i]);
            cout << endl;
        }
    }
    
    // =============== 블록의 구성요소 출력 =============== //
    void
    Network::PrintBlock(string blockName, Block block) {
        cout << blockName << ": ";
        cout << "{" << endl;
        PrintVector("   prevHash", block.header.prevHash);
        PrintTxs("   txs", block.body.txs);
        cout << "}" << endl;
    }

    // =============== 벡터의 구성요소 출력 (uint8_t) =============== //
    void
    Network::PrintVector(string vecName, vector<uint8_t> vec) {
        cout << vecName << ": ";
        for(int i = 0; i < vec.size(); i++) { cout << vec[i]; }
        cout << endl;
    }
    
    void
    Network::PrintTxs(string txName, vector<Tx> txs) {
        for (size_t i = 0; i < txs.size(); i++) { 
            cout << "   tx[" << i << "] { " << endl;
            txs[i].PrintTx(); 
        }
        cout << "}" << endl;
    }

    // =============== 벡터의 구성요소 출력 (int) =============== //
    void
    Network::PrintIntVector(string vecName, vector<int> vec) {
        cout << vecName << ": ";
        for(int i = 0; i < vec.size(); i++) { cout << vec[i]; }
        cout << endl;
    }

    // =============== Convert Bytes(vector) to Packet(array) =============== //  
    uint8_t *
    Network::Bytes2Packet(vector<uint8_t> vec, int vecSize) {
        uint8_t * packet = (uint8_t *)malloc((vecSize + 1) * sizeof(uint8_t));
        for (int i = 0; i < vecSize; i++) { packet[i] = vec[i]; }
        packet[vecSize] = '\0';

        return packet;
    }

    // =============== Convert Bytes(vector) to Array =============== //  
    void
    Network::Bytes2Array(vector<uint8_t> bytes, uint8_t * array) {
        const char *temp;
        char ch;

        string str = Bytes2String(bytes);
        for (size_t i = 0; i < str.size(); i += 2) {
            temp = str.substr(i, 2).c_str();
            ch = stoul(temp, NULL, 16);
            array[i/2] = ch;
        }

        // array[str.size()/2] = '\0';
    }

    // =============== Convert bytes to string =============== //
    string
    Network::Bytes2String(vector<uint8_t> vec) {
        string str = "";
        for (size_t i = 0; i < vec.size(); i++) { str += vec[i]; }

        return str;
    }

    void Network::String2Array(string str, uint8_t *array) {
        const char *temp;

        char ch;
        for (size_t i = 0; i < str.size(); i += 2) {
            temp = str.substr(i, 2).c_str();
            ch = stoul(temp, NULL, 16);
            array[i/2] = ch;
        }
    }

    // =============== Convert Array to Bytes(vector) =============== //  
    vector<uint8_t>
    Network::Array2Bytes(uint8_t arr[], int arrSize) {
        stringstream s;
        s << setfill('0') << hex;
        for (int i = 0; i < arrSize; i++)       { s << setw(2) << (unsigned int)arr[i]; }
        
        string str = s.str();
        vector<uint8_t> bytes;
        for (size_t i = 0; i < str.size(); i++)    { bytes.push_back(str[i]); }
        
        return bytes;
    }

// ===================================================================================================================================================


    Blockchain::Blockchain(void) {

    }

    Blockchain::~Blockchain(void) {
        // NS_LOG_FUNCTION(this);
    }

    // =============== 체인 끝에 블록을 추가 =============== //
    void
    Blockchain::AddBlock(Block newBlock) {
        this->blocks.push_back(newBlock);
    }

    // =============== 현재 체인의 길이를 계산 =============== //
    int
    Blockchain::GetBlockchainHeight(void) {
        return this->blocks.size();
    }

    // =============== 최신 블록을 리턴 =============== //
    Block
    Blockchain::GetLatestBlock(void) {
        return this->blocks[this->GetBlockchainHeight() - 1];
    }

// ===================================================================================================================================================


    Block::Block(void) {

    }

    Block::Block(vector<uint8_t> prevHash, vector<Tx> txs) {
        this->SetHeader(prevHash);
        this->SetBody(txs);
    }

    Block::~Block(void) {
        // NS_LOG_FUNCTION(this);
    }

    void
    Block::SetHeader(vector<uint8_t> prevHash) {
        this->header.prevHash = prevHash;
    }

    void
    Block::SetBody(vector<Tx> txs) {
        this->body.txs = txs;
    }

    Block::BlockHeader::BlockHeader(void) {
        // NS_LOG_FUNCTION(this);
    }

    Block::BlockHeader::~BlockHeader(void) {

    }

    Block::BlockBody::BlockBody(void) {

    }

    Block::BlockBody::~BlockBody(void) {
        // NS_LOG_FUNCTION(this);
    }


    pair<vector<uint8_t>, int>
    Block::SerializeBlock(void) {
        vector<uint8_t> serializedPrevHash = this->SerializePrevHash().first;
        vector<uint8_t> serializedTxs = this->SerializeTxs().first;
        
        int serializedPrevHashSize = this->SerializePrevHash().second;
        int serializedTxsSize = this->SerializeTxs().second;

        vector<uint8_t> serializedBlock;
        serializedBlock.push_back('/');
        serializedBlock.push_back('b');
        for (int i = 0; i < serializedPrevHashSize; i++)    { serializedBlock.push_back(serializedPrevHash[i]); }
        for (int i = 0; i < serializedTxsSize; i++)         { serializedBlock.push_back(serializedTxs[i]); }

        return make_pair(serializedBlock, serializedBlock.size());
    }

    pair<vector<uint8_t>, int>
    Block::SerializePrevHash(void) {
        vector<uint8_t> serializedPrevHash;
        
        serializedPrevHash.push_back('/');
        serializedPrevHash.push_back('h');
        for (size_t i = 0; i < this->header.prevHash.size(); i++) { serializedPrevHash.push_back(this->header.prevHash[i]); }

        return make_pair(serializedPrevHash, serializedPrevHash.size());
    }

    pair<vector<uint8_t>, int>
    Block::SerializeTxs(void) {
        vector<uint8_t> serializedTxs;

        for (size_t i = 0; i < this->body.txs.size(); i++) { 
            vector<uint8_t> serializedTx = this->body.txs[i].SerializeTx().first;
            int serializedTxSize = this->body.txs[i].SerializeTx().second;
            for (int j = 0; j < serializedTxSize; j++) { 
                serializedTxs.push_back(serializedTx[j]); 
            }
        }

        return make_pair(serializedTxs, serializedTxs.size());
    }

    // =============== Deserialize Block =============== //
    void
    Block::DeserializeBlock(string packet) {
        char offset = NULL;
        int idx = -1;
        int i = -1;
        do {
            i++;
            if (packet[i] == '/') {
                if      (packet[i + 1] == 'b')  { offset = 'b'; i++;                                             continue; } // 'b'lock
                else if (packet[i + 1] == 'h')  { offset = 'h'; i++;                                             continue; } // prev'h'ash
                else if (packet[i + 1] == 'x')  { offset = 'x'; i++; idx++; Tx tx; this->body.txs.push_back(tx); continue; } // t'x'
                else if (packet[i + 1] == 'k')  { offset = 'k'; i++;                                             continue; } // 'k'ey
                else if (packet[i + 1] == 's')  { offset = 's'; i++;                                             continue; } // 's'ign
                
            } else if (packet[i] == '\0') { break; }

            switch(offset) {
                // 'b'lock
                case 'b':
                    continue;

                // prev'h'ash
                case 'h':
                    this->header.prevHash.push_back(packet[i]);
                    continue;

                // t'x'
                case 'x':
                    this->body.txs[idx].txItem.push_back(packet[i]);
                    continue;
                    
                // 'k'ey
                case 'k':
                    this->body.txs[idx].pk.push_back(packet[i]);
                    continue;

                // 's'ign
                case 's':
                    this->body.txs[idx].sign.push_back(packet[i]);
                    continue;

            }
        } while(i != packet.size());
    }
// ===================================================================================================================================================

    // =============== Tx Constructor =============== //
    Tx::Tx(void) {

    }

    // =============== Tx Destructor =============== //
    Tx::~Tx(void) {
        // NS_LOG_FUNCTION(this);
    }


    // void                            
    // Tx::PrintTx(void) {

    // }

    pair<vector<uint8_t>, int>      
    Tx::SerializeTx(void) {
        vector<uint8_t> serializedTx;

        serializedTx.push_back('/');
        serializedTx.push_back('x');
        for (size_t i = 0; i < this->txItem.size(); i++) { serializedTx.push_back(this->txItem[i]); }
         
        serializedTx.push_back('/');
        serializedTx.push_back('s');
        for (size_t i = 0; i < this->sign.size(); i++) { serializedTx.push_back(this->sign[i]); }

        serializedTx.push_back('/');
        serializedTx.push_back('k');
        for (size_t i = 0; i < this->pk.size(); i++) { serializedTx.push_back(this->pk[i]); }

        return make_pair(serializedTx, serializedTx.size());
    }

    // void                            
    // Tx::DeserializeTx(string packet) {

    // }

    int                             
    Tx::GetTxSize(void) {
        return this->txItem.size() + this->sign.size() + this->pk.size();
    }

    // =============== Print Transactions =============== //
    void
    Tx::PrintTx(void) {
        cout << "       txItem: ";
        for (size_t i = 0; i < this->txItem.size(); i++)   { cout << this->txItem[i]; }
        cout << endl;
        cout << "       sign: ";
        for (size_t i = 0; i < this->sign.size(); i++)     { cout << this->sign[i]; }
        cout << endl;
        cout << "       pk: ";
        for (size_t i = 0; i < this->pk.size(); i++)       { cout << this->pk[i]; }
        cout << endl;
    }

// ===================================================================================================================================================


    TEE::TEE(void) {

    }

    TEE::~TEE(void) {
        // NS_LOG_FUNCTION(this);
    }


// ===================================================================================================================================================


    PoET::PoET(void) {

    }

    PoET::~PoET(void) {
        // NS_LOG_FUNCTION(this);
    }

    // =============== TypeId 설정 =============== //
    TypeId
    PoET::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::PoET")                                                     // Object의 TypeId
        .SetParent<Application>()                                                                   // Aggregation Object Mechanism
        .SetGroupName("Applications")                                                               // tid가 포함된 그룹의 이름
        .AddConstructor<PoET>();                                                                    // Abstract Object Factory Mechanism
        
        return tid;
    }    

    void
    PoET::StartApplication() {
        InitializeKey();
        InitializeChain();
        InitializeMempool();

        // ===== socket 생성 및 설정 ===== //           
        if(!m_socket) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");                             // TypeId 설정
            m_socket = Socket::CreateSocket(GetNode(), tid);                                        // TypeId로 socket 생성
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 7071);               // IP 주소와 포트 번호(7071)로부터 소켓의 주소 생성
            m_socket->Bind(local);                                                                  // 소켓에 ip와 포트(7071) 할당
            m_socket->Listen();                                                                     // 클라이언트가 소켓에 연결할 수 있도록 수신 대기 상태로 설정
        }

        m_socket->SetRecvCallback(MakeCallback(&PoET::HandleRead, this));                           // Packet이 왔을 때 실행되는 함수 설정
        m_socket->SetAllowBroadcast(true);                                                          // 브로드캐스트가 가능하도록 설정

        // ===== peer node와의 소켓 연결 ===== //               
        vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();                              // 모든 인접 노드의 주소를 불러오기 위한 Iterator
        while(iter != m_peersAddresses.end()){                                                      // 모든 인접 노드와 연결
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");                             // TypeId 설정
            Ptr<Socket> socketClient = Socket::CreateSocket(GetNode(), tid);                        // 해당 TypeId로 client socket 생성
            socketClient->Connect(InetSocketAddress(*iter, 7071));                                  // *iter에 해당하는 노드와 연결
            m_peersSockets[*iter] = socketClient;                                                   // m_peersSockets에 socketClient 저장
            iter++;                                                                                 // 다음 peer node
        }               

        network.PrintChain("genesisChain", m_chain);
        sleep(1);

        Simulator::Schedule(Seconds(0), &PoET::NewRound, this);
    }

    void
    PoET::StopApplication() {
        // NS_LOG_FUNCTION(this);
    }

    void
    PoET::HandleRead(Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        string msg;

        // ===== packet에 대한 동작 과정 ===== //
        while((packet = socket->RecvFrom(from))) {
            socket->SendTo(packet, 0, from);
            if (packet->GetSize() == 0) { break; }                                                  // 패킷이 비어있는지 검증
            
            if (InetSocketAddress::IsMatchingType(from)) {
                msg = GetPacketContent(packet, from);                                               // 수신받은 패킷의 내용 Get
                Block block;
                block.DeserializeBlock(msg);

                VerifyTxs(block);

                if(ZTest()) {
                    Simulator::Cancel(m_eventId);
                    m_chain.AddBlock(block);
                }
                
                cout << "============================================= Receive Block ==========================================" << endl;

                network.PrintChain("Node " + to_string(GetNode()->GetId()), m_chain);
                Simulator::Schedule(Seconds(0), &PoET::NewRound, this);

                cout << "======================================================================================================" << endl;

                sleep(1);
            }
        }
    }        
    
    // =============== 패킷 브로드캐스팅 =============== //
    void
    PoET::SendPacket(Ptr<Socket> socketClient, Ptr<Packet> p) {
        socketClient->Send(p);
    }

    // =============== 블록 브로드캐스팅 =============== //
    void
    PoET::SendBlock(Block block) {
        Ptr<Packet> p;

        int dataSize = block.SerializeBlock().second;
        uint8_t *data = network.Bytes2Packet(block.SerializeBlock().first, dataSize);
        p = Create<Packet>(data, dataSize);                                                // 패킷 생성

        network.fastestNode = GetNode()->GetId();

        cout << "============================================= Send Block ============================================" << endl;
        cout << endl << "fastestNode: " << network.fastestNode << endl;

        if (ZTest()) {
            network.winNum[network.fastestNode] += 1;
            m_chain.AddBlock(block);
            network.PrintIntVector("winNum", network.winNum);

            // ===== 모든 노드에게 브로드캐스팅 ===== //
            vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();                              // 시작 peer node부터
            while(iter != m_peersAddresses.end()) {                                                     // 마지막 peer node까지
                Ptr<Socket> socketClient = m_peersSockets[*iter];                                       // 소켓 생성                               
                Simulator::Schedule(Seconds(0), &PoET::SendPacket, this, socketClient, p);              // 패킷 송신
                iter++;                                                                                 // 다음 peer node
            }

            Simulator::Schedule(Seconds(1), &PoET::NewRound, this);
        }

        else {
            cout << "Z-Test failed" << endl;
            return;
        }
        
        network.PrintChain("Fastest Chain", m_chain);
        cout << "=====================================================================================================" << endl;

        sleep(1);
    }

    // =============== 패킷 내용 Get =============== //
    string
    PoET::GetPacketContent(Ptr<Packet> packet, Address from) {
        char *packetInfo = new char[packet->GetSize() + 1];
        ostringstream totalStream;
        packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo), packet->GetSize());
        packetInfo[packet->GetSize()] = '\0';
        totalStream << m_bufferedData[from] << packetInfo;
        string totalReceivedData(totalStream.str());

        return totalReceivedData;
    }

    // =============== 블록의 해시값 계산 =============== //
    vector<uint8_t>
    PoET::GetBlockHash(Block block) {
        string str;
        
        for (int i = 0; i < network.prevHashSize; i++)  { str += block.header.prevHash[i]; }
        for (int i = 0; i < block.body.txs.size(); i++) { 
            for (int j = 0; j < block.body.txs[i].txItem.size(); j++)   { str += block.body.txs[i].txItem[j]; }
            for (int k = 0; k < block.body.txs[i].sign.size(); k++)     { str += block.body.txs[i].sign[k]; }
            for (int l = 0; l < block.body.txs[i].pk.size(); l++)       { str += block.body.txs[i].pk[l]; }
        }

        SHA sha;
        string temp;
        vector<uint8_t> blockHashValue;

        sha.update(str);
        temp = SHA::toString(sha.digest());

        for (int i = 0; i < network.prevHashSize; i++) { blockHashValue.push_back(temp[i]); }

        return blockHashValue;
    }

    // =============== PendingTxs에 Tx 추가 =============== //
    void
    PoET::AddPendingTxs(Tx pendingTx) {
        m_memPool.push(pendingTx);
    }

    // =============== Get Pending Transactions from Memory Pool =============== //
    vector<Tx>
    PoET::GetPendingTxs(void) {
        
        vector<Tx> newTxs;
        int txStorage = network.txStorage;

        while(true) {
            if (m_memPool.empty()) { 
                cout << "Memory Pool is empty" << endl; 
                break;
            }

            Tx tx = m_memPool.front();
            int txSize = tx.GetTxSize();
            if (txSize <= txStorage) {
                m_memPool.pop();
                newTxs.push_back(tx);
                txStorage = txStorage - txSize;
            } else { break; }
        }

       return newTxs;
    }

    float
    PoET::GetWaitTime(void) {
        float r = (float)dis(gen) / dis.max();                                                                             // r ∈ [0, 1]

        cout << "====================================== Generate Random Value ========================================" << endl;


        cout << endl << "Random Value: " << r << endl;
        // return network.minimumWait - network.localAverage * log10(r);                                            // Random Wait Time 리턴
        return network.minimumWait - network.localAverage * log(r);                                                 // Random Wait Time 리턴
    }

    void
    PoET::NewRound(void) {
        m_waitTime = GetWaitTime();
        
        cout << "Node[" << GetNode()->GetId() <<"]s waitTime: " << m_waitTime << endl;
        vector<uint8_t> prevHash = GetBlockHash(m_chain.GetLatestBlock());
        vector<Tx> txs = GetPendingTxs();
        Block newBlock(prevHash, txs);

        // network.PrintBlock("newBlock", newBlock);
        m_eventId = Simulator::Schedule(Seconds(m_waitTime), &PoET::SendBlock, this, newBlock);

        cout << "=====================================================================================================" << endl;

        sleep(1);
    }

    // ========== 노드가 비정상적으로 블록을 많이 생성했는지에 대해 검증하는 함수 ========== //
    bool
    PoET::ZTest(void) {
        int m = m_chain.GetBlockchainHeight();                                                              // 네트워크 내의 총 블록 개수
        float p = 1.0 / network.numNodes;                                                                   // 각 노드의 블록 생성 확률 (모든 노드가 동일)
        float zScore = (network.winNum[network.fastestNode] - m * p) / sqrt(m * p * (1 - p));               // z-score 계산

        NS_LOG_INFO("zScore: " << zScore);                                                                  // z-score 출력

        if (zScore <= network.zMax) { return true; }      
        else { return false; }                                                                              // z_score가 zmax를 초과하였을 경우 블록 생성 실패
    }

    void
    PoET::InitializeChain(void) {
        Block genesisBlock;
        Tx genesisTx = GenerateGenesisTx();

        genesisBlock.header.prevHash.assign(network.prevHashSize, '0');
        genesisBlock.body.txs.push_back(genesisTx);

        m_chain.AddBlock(genesisBlock);
    }

    // =============== Initialize Memory Pool =============== //
    void
    PoET::InitializeMempool(void) {
        for (int i = 0; i < network.numTxs; i++) {
            Tx tx = GenerateTx();
            m_memPool.push(tx);
        }
    }

    // =============== Initialize Key Pair =============== //
    void
    PoET::InitializeKey(void) {
        uint8_t public_key[OQS_SIG_dilithium_2_length_public_key];
        uint8_t secret_key[OQS_SIG_dilithium_2_length_secret_key];

        OQS_SIG_dilithium_2_keypair(public_key, secret_key);

        OQS_STATUS rc = OQS_SIG_dilithium_2_keypair(public_key, secret_key);
        m_pk = network.Array2Bytes(public_key, OQS_SIG_dilithium_2_length_public_key);
        m_sk = network.Array2Bytes(secret_key, OQS_SIG_dilithium_2_length_secret_key);
    }

    // =============== Generate Genesis Transaction =============== //
    Tx
    PoET::GenerateGenesisTx(void) {
        uint8_t arr[11];
        network.String2Array("43727970746f4372616674", arr);
        vector<uint8_t> genesisTxItem = network.Array2Bytes(arr, 11);

        Tx genesisTx;
        genesisTx.txItem = genesisTxItem;
        genesisTx.sign = SignTx(genesisTx.txItem, m_sk);
        genesisTx.pk = m_pk;

        return genesisTx;
    }

    // =============== Generate Transaction =============== //
    Tx
    PoET::GenerateTx(void) {
        stringstream randomValue;
        Tx tx;
        int txSize = (dis(gen) % network.maxTxItemSize);
        // int txSize = network.maxTxItemSize;
        if (txSize % 2 == 1) { txSize += 1; }
        if (txSize == 0) { txSize += 2; }
        for (int i = 0; i < txSize; i++) {
            randomValue << hex << dis(gen) % 16;
            tx.txItem.push_back(randomValue.str()[i]);
        }

        tx.sign = SignTx(tx.txItem, m_sk);
        tx.pk = m_pk;
        
        return tx;
    }

    vector<uint8_t>
    PoET::SignTx(vector<uint8_t> txItem, vector<uint8_t> sk) {
        uint8_t signature[OQS_SIG_dilithium_2_length_signature] = {};
        size_t signature_len;

        uint8_t secret_key[OQS_SIG_dilithium_2_length_secret_key] = {};
        network.Bytes2Array(sk, secret_key);
        // secret_key[OQS_SIG_dilithium_2_length_secret_key] = '\0';

        size_t message_len = txItem.size()/2;
        uint8_t message[message_len] = {};
        network.Bytes2Array(txItem, message);
        // message[message_len] = '\0';
        
        OQS_SIG_dilithium_2_sign(signature, &signature_len, message, message_len, secret_key);
        // signature[signature_len] = '\0';
        vector<uint8_t> sig = network.Array2Bytes(signature, OQS_SIG_dilithium_2_length_signature);        
        return sig;
    }

    bool
    PoET::VerifyTxs(Block block) {
        for (int i = 0; i < block.body.txs.size(); i++) {
            Tx tx = block.body.txs[i];
            VerifyTx(tx.txItem, tx.sign, tx.pk);
        }

        return true;
    }

    bool
    PoET::VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> pk) {
        size_t message_len = txItem.size()/2;
        uint8_t message[message_len];
        network.Bytes2Array(txItem, message);
        // message[message_len] = '\0';

        size_t signature_len = sign.size()/2;
        uint8_t signature[signature_len];
        network.Bytes2Array(sign, signature);
        // signature[signature_len] = '\0';

        uint8_t public_key[OQS_SIG_dilithium_2_length_public_key];
        network.Bytes2Array(pk, public_key);
        // public_key[OQS_SIG_dilithium_2_length_public_key] = '\0';

        OQS_STATUS rc;
        rc = OQS_SIG_dilithium_2_verify(message, message_len, signature, signature_len, public_key);

        
        if (rc != OQS_SUCCESS) { 
            cout << "VerifyTx Failed" << endl;
            return OQS_ERROR;
        }
        else { 
            cout << "VerifyTx Successedzz" << endl; 
            return OQS_SUCCESS;
        }
    }
}