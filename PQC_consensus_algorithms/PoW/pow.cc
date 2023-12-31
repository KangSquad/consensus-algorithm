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
#include "pow.h"
#include "stdlib.h"
#include "ns3/ipv4.h"
#include "SHA256.h"
#include <ctime>
#include <random>
#include <map>
#include <unistd.h>
#include <sstream>
#include <iomanip>



using namespace std;


random_device rd; // 난수 생성기
mt19937 gen (rd ());
uniform_int_distribution<int> dis (0, 999999);


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("PoW");
    NS_OBJECT_ENSURE_REGISTERED (PoW);
    
    static Network network;
    
// ===================================================================================================================================================

    // =============== Network Class Constructor =============== //
    Network::Network (void)
    {
        difficulty = 4;                             // 블록 채굴 난이도
        blockSize = 20480 * 2.5;                    // 20480 = 10 KB
        headerSize = 76;                            // 76 bytes
        txStorage = blockSize - headerSize;         // 트랜잭션을 넣을 수 있는 블록 내의 공간
        maxTxItemSize = 512;                        // 256 bytes
    }
    
    // =============== Network Class Destructor =============== //
    Network::~Network (void)
    {
    }

    // =============== Convert int to char =============== //
    char Network::IntToChar (int num)
    {
        return num + '0';
    }

    // =============== Convert char to int =============== //
    char Network::CharToInt (char num)
    {
        return num - '0';
    }

    // =============== Convert string to bytes =============== //
    vector<uint8_t> Network::String2Bytes(string str) 
    {
        vector<uint8_t> vec;
        for (int i = 0; i < str.size(); i++) 
        {
            vec.push_back(str[i]); 
        }

        return vec;
    }

    // =============== Generate the previous block hash value of the genesis block =============== //
    vector<uint8_t> Network::GetGenesisPrevHash (void)
    {
        string Genesis_prevHash = "";
    
        for (int i = 0; i < 64; i++)
        {
            Genesis_prevHash = Genesis_prevHash + "0";          // Since the hash value of the previous block does not exist in the genesis block, it is randomly initialized.
        }

        return network.String2Bytes(Genesis_prevHash);
    }
    
    // =============== Get Hash Value =============== //
    string Network::GetHash (string input)
    {
        SHA sha;
        sha.update (input);
        string output = SHA::toString (sha.digest ());
    
        return output;
    }
    
    // =============== Check if mining =============== //
    bool Network::CheckHash (string output)
    {
        for (int i = 0; i < network.difficulty; i++)
        {
            if (output[i] != '0')
            {
                return false;
            }
        }
        return true;
    }
    
    // =============== Count the number of digits. =============== //
    int Network::CountIntlen (int num)
    {
        //int len = num;
        int count = 0;
        
        do
        {
            num = int (num / 10);
            count++;
        } while (num > 0);
    
        return count;
    }

    // =============== Convert Bytes(vector) to Packet(array) =============== //  
    uint8_t * Network::Bytes2Packet(vector<uint8_t> vect) {
        int vecSize = vect.size();
        uint8_t * packet = (uint8_t *)malloc(vecSize+1 * sizeof(uint8_t));
        for (int i = 0; i < vecSize; i++) { packet[i] = vect[i]; }
        packet[vecSize] = '\0';

        return packet;
    }

    vector<uint8_t> Block::SerializeId(void)
    {
        vector<uint8_t> SerializedBlockId;

        int blockId = this->header.blockId;
        int blockId_len = network.CountIntlen (blockId);

        for (int idx = 0; idx < blockId_len; idx++)
        {
            SerializedBlockId.push_back(network.IntToChar (blockId % 10));
            blockId = blockId / 10;
        }

        SerializedBlockId.push_back('i');         // Block'i'd
        SerializedBlockId.push_back('/');

        reverse(SerializedBlockId.begin(), SerializedBlockId.end());

        return SerializedBlockId;
    }

    vector<uint8_t> Block::SerializePrevHash(void)
    {
        vector<uint8_t> SerializedPrevHash;

        SerializedPrevHash.push_back('/');
        SerializedPrevHash.push_back('h'); //prev'h'ash
        
        for (int i=0; i < this->header.prevHash.size (); i++)
        {
            SerializedPrevHash.push_back(header.prevHash[i]);
        }

        return SerializedPrevHash;
    }
    
    vector<uint8_t> Block::SerializeDifficulty(void)                                             // Serialize difficulty
    {
        vector<uint8_t> SerializedDifficulty;

        int difficulty = this->header.difficulty;
        int difficulty_len = network.CountIntlen (difficulty);

        for (int idx = 0; idx < difficulty_len; idx++)
        {
            SerializedDifficulty.push_back(network.IntToChar (difficulty % 10));
            difficulty = difficulty / 10;
        }

        SerializedDifficulty.push_back('d'); // 'd'ifficulty
        SerializedDifficulty.push_back('/');

        reverse(SerializedDifficulty.begin(), SerializedDifficulty.end());
    

        return SerializedDifficulty;
    }
    
    vector<uint8_t> Block::SerializeNonce(void)                                                  // Serialize nonce
    {
        vector<uint8_t> SerializedNonce;

        int nonce = this->header.nonce;
        int nonce_len = network.CountIntlen (nonce);

        for (int idx = 0; idx < nonce_len; idx++)
        {
            SerializedNonce.push_back(network.IntToChar (nonce % 10));
            nonce = nonce / 10;
        }

        SerializedNonce.push_back('n'); // 'n'once
        SerializedNonce.push_back('/');

        reverse(SerializedNonce.begin(), SerializedNonce.end());


        return SerializedNonce;
    }

    vector<uint8_t> Block::SerializeTxs(void)                                                    // Serialize txs
    {
        vector<uint8_t> SerializedTxs;

        for (int k=0; k < this->body.txs.size(); k++)
        {
            SerializedTxs.push_back('/');
            SerializedTxs.push_back('t');    // 't'ransaction
            for ( int j=0; j < this->body.txs[k].txItem.size(); j++)
            {
                SerializedTxs.push_back(this->body.txs[k].txItem[j]);
            }

            SerializedTxs.push_back('/');
            SerializedTxs.push_back('s');    // 's'ign
            for ( int j=0; j < this->body.txs[k].sign.size(); j++)
            {
                SerializedTxs.push_back(this->body.txs[k].sign[j]);
            }

            SerializedTxs.push_back('/');
            SerializedTxs.push_back('p');    // 'p'k
            for ( int j=0; j < this->body.txs[k].pk.size(); j++)
            {
                SerializedTxs.push_back(this->body.txs[k].pk[j]);
            }
        }

        return SerializedTxs;
    }

    // =============== Serialize Block =============== //
    vector<uint8_t> Block::SerializeBlock (void)
    {
    
        vector<uint8_t> serializeId = this->SerializeId();
        vector<uint8_t> serializePrevhash = this->SerializePrevHash();
        vector<uint8_t> serializeDifficulty = this->SerializeDifficulty();
        vector<uint8_t> serializeNonce = this->SerializeNonce();
        vector<uint8_t> serializeTxs = this->SerializeTxs();


        vector<uint8_t> serializedBlock;

        serializedBlock.push_back('/');
        serializedBlock.push_back('b');    // 'b'lock
        for (int i=0; i< serializeId.size();i++)            {  serializedBlock.push_back(serializeId[i]);}
        for (int i=0; i< serializePrevhash.size();i++)      {  serializedBlock.push_back(serializePrevhash[i]);}
        for (int i=0; i< serializeDifficulty.size();i++)    {  serializedBlock.push_back(serializeDifficulty[i]);}
        for (int i=0; i< serializeNonce.size();i++)         {  serializedBlock.push_back(serializeNonce[i]);}
        for (int i=0; i< serializeTxs.size();i++)           {  serializedBlock.push_back(serializeTxs[i]);}


        return serializedBlock;
    }

    // =============== Deserialize Block =============== //
    void Block::DeserializeBlock (string packet)
    {
        char offset;
        int idx = -1;
        int i = -1;
                
        do 
        {
            i++;
            if (packet[i] == '/') 
            {
                if      (packet[i + 1] == 'b')  { offset = 'b'; i++;                                               continue; } // 'b'lock
                else if (packet[i + 1] == 'i')  { offset = 'i'; i++;                                               continue; } // block'i'd
                else if (packet[i + 1] == 'h')  { offset = 'h'; i++;                                               continue; } // prev'h'ash
                else if (packet[i + 1] == 'd')  { offset = 'd'; i++;                                               continue; } // 'd'ifficulty
                else if (packet[i + 1] == 'n')  { offset = 'n'; i++;                                               continue; } // 'n'once
                else if (packet[i + 1] == 't')  { offset = 't'; i++; idx++; Tx tx; this->body.txs.push_back(tx);   continue; } // 't'ransaction
                else if (packet[i + 1] == 's')  { offset = 's'; i++;                                               continue; } // 's'ign
                else if (packet[i + 1] == 'p')  { offset = 'p'; i++;                                               continue; } // 'k'ey
            } 
            else if (packet[i] == '\0') 
            { 
                break; 
            }      

            switch(offset) 
            {
                // 'b'lock
                case 'b':
                    break;
                
                // block'i'd
                case 'i':
                {
                    string str_blockid="";
                    while(packet[i]!='/')
                    {
                        str_blockid += packet[i];
                        i++;
                    }
                    this->header.blockId = stoi(str_blockid);
                    i--;

                    break;
                }

                // prev'h'ash
                case 'h':
                    this->header.prevHash.push_back(packet[i]);
                    break;

                // 'd'ifficulty
                case 'd':
                {
                    string str_difficulty="";
                    while(packet[i]!='/')
                    {
                        str_difficulty += packet[i];
                        i++;
                    }
                    this->header.difficulty = stoi(str_difficulty);
                    i--;
                    break;
                }
                    
                // 'n'once
                case 'n':
                {
                    string str_nonce="";
                    while(packet[i]!='/')
                    {
                        str_nonce += packet[i];
                        i++;
                    }
                    this->header.nonce = stoi(str_nonce);
                    i--;
                    break;
                }

                // 't'x item
                case 't':
                    this->body.txs[idx].txItem.push_back(packet[i]);
                    break;

                // 's'ign
                case 's':
                    this->body.txs[idx].sign.push_back(packet[i]);
                    break;
                
                // 'p'ublickey
                case 'p':
                    this->body.txs[idx].pk.push_back(packet[i]);
                    break;
            }  

        } while(i!=packet.size());

    }
    
    // =============== Convert Array to Bytes(vector) =============== //  
    vector<uint8_t> Network::Array2Bytes(uint8_t arr[], int arrSize) 
    {
        stringstream s;
        s << setfill('0') << hex;
        for (int i = 0; i < arrSize; i++)       { s << setw(2) << (unsigned int)arr[i]; }
        
        string str = s.str();
        vector<uint8_t> bytes;
        for (int i = 0; i < str.size(); i++)    { bytes.push_back(str[i]); }
        
        return bytes;
    }

    // =============== Convert Bytes(vector) to Array =============== //  
    void Network::Bytes2Array(vector<uint8_t> bytes, uint8_t * array) 
    {
        const char *temp;
        char ch;

        string str = Bytes2String(bytes);
        for (int i = 0; i < str.size(); i += 2) 
        {
            temp = str.substr(i, 2).c_str();
            ch = stoul(temp, NULL, 16);
            array[i/2] = ch;
        }
        // array[str.size()/2] = '\0';
    }

    // =============== Convert bytes to string =============== //
    string Network::Bytes2String(vector<uint8_t> vec) 
    {
        string str = "";
        for (int i = 0; i < vec.size(); i++) 
        { 
            str += vec[i]; 
        }

        return str;
    }

    // =============== Print Bytes =============== //
    void Network::PrintBytes(vector<uint8_t> vec) 
    {
        for (int i = 0; i < vec.size(); i++) 
        { 
            cout << vec[i]; 
        }
        cout << endl;
    }

    // =============== Broadcast Packet =============== //
    void SendPacket (Ptr<Socket> socketClient, Ptr<Packet> p)
    {
        socketClient->Send (p);
    }

    // =============== Blockchain Class Constructor =============== //
    Blockchain::Blockchain (void)
    {
    }

    // =============== Blockchain Class Destructor =============== //
    Blockchain::~Blockchain (void)
    {
    }

    // =============== Add Block to Blockchain =============== //
    void Blockchain::AddBlock (Block newBlock)
    {
        m_blocks.push_back (newBlock);
    }

    // =============== Print Block =============== //
    void Blockchain::PrintBlockchain (int idx)
    {
        cout << "Block " << idx <<endl;
        cout << "{" << endl << "   BlockId : " << this -> m_blocks[idx].header.blockId <<endl;
        cout << "   PrevHash : ";
        for(int i=0; i < this->m_blocks[idx].header.prevHash.size(); i++)
        {
            cout <<  this->m_blocks[idx].header.prevHash[i];
        }
        cout << endl;
        cout << "   Difficulty : " << this->m_blocks[idx].header.difficulty << endl;
        cout << "   Nonce : " << this->m_blocks[idx].header.nonce << endl;

        // for (int i = 0; i < network.tx_size; i++)
        string txItem_str = "";
        string sign_str = "";
        string pk_str = "";

        for (int i = 0; i < this -> m_blocks[idx].body.txs.size(); i++)
        {
            txItem_str = "";
            sign_str = "";
            pk_str = "";
            
            for (int j = 0; j < this->m_blocks[idx].body.txs[i].txItem.size(); j++)
            {
                txItem_str += this -> m_blocks[idx].body.txs[i].txItem[j];
            }

            for (int j = 0; j < this->m_blocks[idx].body.txs[i].sign.size(); j++)
            {
                sign_str += this -> m_blocks[idx].body.txs[i].sign[j];
            }

            for (int j = 0; j < this->m_blocks[idx].body.txs[i].pk.size(); j++)
            {
                pk_str += this -> m_blocks[idx].body.txs[i].pk[j];
            }

            cout << "   <Transaction " << to_string(i) << ">" << endl;
            cout << "   {" <<endl;
            cout << "      txItem : " << txItem_str << endl;
            // cout << "      sign : " << sign_str << endl;
            // cout << "      pk : " << pk_str << endl;
            cout << "   }\n";
            
        }

    }

    // =============== Print Chain =============== //
    void Blockchain::PrintChain(void)
    {
        cout << "==========================Blockchain==========================" << endl;
        for(int i=0; i<GetBlockChainHeight(); i++)
        {
            PrintBlockchain(i);
        }
        cout << "==============================================================" << endl;
    }
    
    // =============== Get Chain =============== //
    int Blockchain::GetBlockChainHeight(void)
    {
        return m_blocks.size();
    }
    
    // =============== Block Class Default Constructor =============== // 
    Block::Block (void)
    {
    }

    // =============== Block Class Constructor =============== //
    Block::Block (int blockId, vector<uint8_t> prevHash, int difficulty, int nonce, vector<Tx> tx)
    {
        this->header.blockId = blockId;
        // this->header.prevHash = prevHash;
        this->header.prevHash.resize(prevHash.size());
        copy(prevHash.begin(), prevHash.end(), this->header.prevHash.begin());

        this->header.difficulty = difficulty;
        this->header.nonce = nonce;

        for (int i=0; i < tx.size(); i++)
        {
            this -> body.txs.push_back(tx[i]);
        }

    }

    // =============== Block Class Destructor =============== // 
    Block::~Block (void)
    {
    }

    // =============== Print Block =============== //
    void Block::PrintBlock ()
    {
        
        cout << "Block " << endl << "{" << endl;
        cout << "   BlockId : " << this -> header.blockId << endl;
        cout << "   PrevHash : ";
        for(int i=0; i < this->header.prevHash.size(); i++)
        {
            cout <<  this->header.prevHash[i];
        }
        cout << endl;

        cout << "   Difficulty : " << this -> header.difficulty<< endl;
        cout << "   Nonce : " << this -> header.nonce << endl;

        for (int i = 0; i < this -> body.txs.size(); i++)
        {
            string txItem_str = "";
            string sign_str = "";
            string pk_str = "";
            
            for (int j = 0; j < this -> body.txs[i].txItem.size(); j++)
            {
                txItem_str += this -> body.txs[i].txItem[j];
            }

            for (int j = 0; j < this -> body.txs[i].sign.size(); j++)
            {
                sign_str += this -> body.txs[i].sign[j];
            }

            for (int j = 0; j < this-> body.txs[i].pk.size(); j++)
            {
                pk_str += this -> body.txs[i].pk[j];
            }

            cout << "   <Transaction " << to_string(i) << ">" << endl;
            cout << "   {" <<endl;
            cout << "      txItem : " << txItem_str << endl;
            cout << "      sign : " << sign_str << endl;
            cout << "      pk : " << pk_str << endl;
            cout << "   }" <<endl;
            

        }

        cout << "}" << endl; 
    }
    
    Block::BlockHeader::BlockHeader(void)               // Block Header 생성자
    {
    
    }
    
    Block::BlockHeader::~BlockHeader (void)             // Block Header 소멸자
    {
    
    }
    
    Block::BlockBody::BlockBody(void)               // Block Body 생성자
    {
    
    }
    
    Block::BlockBody::~BlockBody (void)             // Block Body 소멸자
    {
        
    }
    
    // =============== Tx Constructor =============== //
    Tx::Tx(void) {

    }

    // =============== Tx Constructor =============== //
    Tx::Tx(vector<uint8_t> txItem) {
        this->txItem = txItem;
    }

    // =============== Tx Destructor =============== //
    Tx::~Tx(void) {
        NS_LOG_FUNCTION(this);
    }
    
    // =============== Get Transaction Size =============== //
    int Tx::GetTxSize(void) {
        int txItemSize = this->txItem.size();
        int signSize = 0;
        int pkSize = 0;

        int txSize = txItemSize + signSize + pkSize;

        return txSize;
    }

    // =============== Print Transactions =============== //
    void Tx::PrintTx(void) {
        for (int i = 0; i < this->GetTxSize(); i++) 
        { 
            cout << this->txItem[i]; 
        }
        cout << endl;
    }

    vector<uint8_t> PoW::GetBlockHash (Block block)
    {
        string input = "";
        string output;
    
        input += to_string (block.header.blockId);
        input += network.Bytes2String(block.header.prevHash);
        input += to_string (block.header.difficulty);
        input += to_string(block.header.nonce);
    
        output = network.GetHash (input);

        return network.String2Bytes(output);
    
        // return output;
    }
    
    PoW::PoW (void)
    {
    }
    
    PoW::~PoW (void)
    {
        NS_LOG_FUNCTION (this);
    }
    
    TypeId PoW::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::PoW")
                                .SetParent<Application> ()
                                .SetGroupName ("Applications")
                                .AddConstructor<PoW> ();
    
        return tid;
    }
    
    void PoW::StartApplication ()                                                                                                                                                           // 스타트 어플리케이션
    {
        // ===== Initializing ===== //
        InitializeKey();
        InitializeMempool();

        if (!m_socket)                                                                          // 지금 노드에 socket이 없다면, socket 설정
        {
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");                        // TypeId 설정
            
            m_socket = Socket::CreateSocket (GetNode (), tid);                                  // 해당 TypeId로 socket 생성
    
            InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 7071);         // IP 주소와 포트 번호(7071)로부터 소켓의 주소를 만든다
    
    
            m_socket->Bind (local);                                                             // 소켓에 ip와 포트(7071)를 할당
            m_socket->Listen ();                                                                // 대기열에서 대기
        }
        
        m_socket->SetRecvCallback (MakeCallback (&PoW::HandleRead, this));                      // 새로운 데이터를 읽을 수 있을 때 Application에게 알려주는 역할
        m_socket->SetAllowBroadcast (true);                                                     // 브로드캐스트가 가능도록 설정
    
        std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin ();                    // 모든 인접 노드의 주소를 불러오기 위한 Iterator
    
        
        while (iter != m_peersAddresses.end ())                                                 // 모든 인접 노드와 연결
        {
        
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");                        // TypeId 설정
    
            Ptr<Socket> socketClient = Socket::CreateSocket (GetNode (), tid);                  // 해당 TypeId로 client socket 생성
            
            socketClient->Connect (InetSocketAddress (*iter, 7071));                            // *iter에 해당하는 노드와 연결
    
            m_peersSockets[*iter] = socketClient;                                               // m_peersSockets에 다른 노드들의 소켓 저장
    
            iter++;
        }
    
        // NS_LOG_INFO(m_id<<"가 가지고 있는 0 번째 ip 주소와 인덱스 : " << m_peersAddresses[0] << "     " <<  m_peersIndex[m_peersAddresses[0]]   );
        // NS_LOG_INFO(m_id<<"가 가지고 있는 1 번째 ip 주소와 인덱스 : " << m_peersAddresses[1] << "     " <<  m_peersIndex[m_peersAddresses[1]]   );
        // NS_LOG_INFO(m_id<<"가 가지고 있는 2 번째 ip 주소와 인덱스 : " << m_peersAddresses[2] << "     " <<  m_peersIndex[m_peersAddresses[2]]   );
    
        // NS_LOG_INFO(Simulator::Now().GetSeconds());
        // Time t = Time::time();
        // Time::GetTimeStep();
    
        round = 0;

        Block block = GenerateBlock (0);

        m_Blockchain.AddBlock (block);                                                          // 체인에 블록 입력
        round++;
        
        cout << endl;
    
        unsigned int nonce;
        nonce = dis (gen);
        int mineTime = 0;


        // network.start = clock(); //시간 측정 시작
        Simulator::Schedule (Seconds (1), &PoW::RunPoW, this, nonce, mineTime);    // 채굴 시작
    }
    
    void PoW::StopApplication ()
    {
    }
    
    void PoW::HandleRead (Ptr<Socket> socket)                                                        //핸들 리드                               
    {
        // NS_LOG_INFO("Hello world : "<<socket-> GetNode() -> GetId());
        // NS_LOG_INFO(m_id << ": HandleRead");
        // Simulator::Cancel(m_miningId);
        // Simulator::Cancel(m_SuccessMineId);
        // Simulator::Destroy();
    
        Ptr<Packet> packet;
        Address from;
    
        std::string msg;
        
        // RecvFrom을 통해 소켓 내의 데이터 수신
        while ((packet = socket->RecvFrom (from)))
        {
            // 특정 인접 노드에게 데이터 전송
            // SendTo(data, flag, toAddress)
            socket->SendTo (packet, 0, from);
    
            // if EOF라면
            if (packet->GetSize () == 0)
            {
                break;
            }
    
            // from이 Address라면 1, 나머지는 0
            if (InetSocketAddress::IsMatchingType (from))
            {
                msg = GetPacketContent (packet, from);
                // cout << "msg : "<<msg << endl;
    
                // InetSocketAddress inetAddress = InetSocketAddress::ConvertFrom(from);
                // Ipv4Address ipv4Address = inetAddress.GetIpv4();
                // NS_LOG_INFO("m_peerip index : " << m_peersIndex[ipv4Address]);
                // NS_LOG_INFO("m_peerIp : " << ipv4Address);
                // NS_LOG_INFO("from : "<<from);
                // NS_LOG_INFO("msg : "<<msg);

                // cout << "msg : " << msg << endl;
    
                Block receivedBlock;
                receivedBlock.DeserializeBlock (msg);

                // receivedBlock.PrintBlock();
    
                /// 검증하는 코드
                bool validation = ValidationBlock (receivedBlock);
    
                if (validation == true) // 검증 결과 트루면
                {
                    m_Blockchain.AddBlock (receivedBlock); // 블록체인에 블록 추가
                    round ++;
    
                    // m_Blockchain.PrintChain();
    
                }
                
                unsigned int nonce;
                nonce = dis (gen);
                int mineTime = 0;

                // network.end = clock(); //시간 측정 끝
                // double result = (double) (network.end - network.start) / CLOCKS_PER_SEC;
                // cout << "result : " << result << endl;
                // cout << "round : " << round << endl;
    
                Simulator::Schedule(Seconds(1), &PoW::RunPoW, this, nonce, mineTime);
            }
        }
    }
    
    // =============== Initialize Key Pair =============== //
    void PoW::InitializeKey(void) 
    {
        uint8_t public_key[OQS_SIG_dilithium_2_length_public_key];
        uint8_t secret_key[OQS_SIG_dilithium_2_length_secret_key];

        OQS_STATUS rc = OQS_SIG_dilithium_2_keypair(public_key, secret_key);
        m_pk = network.Array2Bytes(public_key, OQS_SIG_dilithium_2_length_public_key);
        m_sk = network.Array2Bytes(secret_key, OQS_SIG_dilithium_2_length_secret_key);
    }

    // =============== Initialize Memory Pool =============== //
    void PoW::InitializeMempool(void) 
    {
        for (int i = 0; i < 500; i++) {
            Tx tx = GenerateTx();
            m_memPool.push(tx);
        }
    }

    // 패킷 내용 수신
    string PoW::GetPacketContent (Ptr<Packet> packet, Address from)
    {
        char *packetInfo = new char[packet->GetSize () + 1];
    
        std::ostringstream totalStream;
    
        packet->CopyData (reinterpret_cast<uint8_t *> (packetInfo), packet->GetSize ());
    
        packetInfo[packet->GetSize ()] = '\0';
    
        totalStream << m_bufferedData[from] << packetInfo;
    
        std::string totalReceivedData (totalStream.str ());
    
        return totalReceivedData;
    }
    
    void PoW::RunPoW (int nonce, int mineTime)                                                                                                                                                      // 블록 채굴
    {
        // NS_LOG_INFO(m_id << ": RunPoW");
        // NS_LOG_INFO ("Node " << GetNode ()->GetId ()<< "의 채굴 시작 시간 :" << (Time) Simulator::Now ().GetSeconds ());
        // NS_LOG_INFO("현재 라운드 : "<<round);
    
        string hRound = to_string (round);
        string hPrevHash = network.Bytes2String(GetBlockHash (m_Blockchain.m_blocks.back ()));
        string hDifficulty = to_string (network.difficulty);
        string hNonce="";
    
        string inputHash="";
        string outputHash="";
    
        hNonce = to_string (nonce);
        inputHash = hRound + hPrevHash + hDifficulty + hNonce; // 헤더 연접 완성
    
        bool checkmine = MiningBlock(inputHash, nonce, mineTime);
    
        if(!checkmine)
        {
            mineTime++;
            nonce++;
            Simulator::Schedule (Seconds(1), &PoW::RunPoW, this, nonce, mineTime); // 채굴 시작
        }
        else
        {
            return;
        }
    }
    
    bool PoW::MiningBlock(string inputHash, int nonce, int mineTime)
    {
    
            // ============== 해시값 계산 ============== //
            string outputHash = network.GetHash (inputHash);
    
            // ======== 해시 출력값 앞자리 0 개수 체크 ======== //
            bool check = network.CheckHash (outputHash);
    
            // ============ 채굴 성공 여부 확인 ========= //
            if (check)
            {
                Simulator::Schedule(Seconds(0), &PoW::SuccessMine, this, nonce, inputHash, outputHash);
    
                return true;
            }
            return false;
              
    }
    
    void PoW::SuccessMine(int nonce, string inputHash, string outputHash)
    {
            cout << "\n========================================================== Round : " << round << " ==========================================================" << endl;
            cout << "< Node " << GetNode ()->GetId () << " : Mining Success >" << endl;
            cout << "input_hash : " << inputHash << endl;
            cout << "output_hash : " << outputHash  << endl << endl;
    
            Block block = GenerateBlock (nonce);
            m_Blockchain.AddBlock (block); // 체인에 블록 입력


            vector<uint8_t> vect_block = block.SerializeBlock ();     // 클래스로 구현된 블록을 arr로 변환

            // uint8_t *arr_block = block.SerializeBlock ();     // 클래스로 구현된 블록을 arr로 변환

            // cout << "===============================================" << endl;
            // cout << "arr_block : " << arr_block << endl;
            // cout << "===============================================" << endl;

            SendBlock(vect_block);
            // SendBlock(arr_block, length);

            // delete arr_block;
            // free(arr_block);

            cout <<"< Node " << GetNode ()->GetId () << " : Broadcast for verification. >" << endl;
            cout << "Block broadcast by Node "<< m_id << " (Round "<< round << ")" << endl;
            m_Blockchain.PrintBlockchain(round);
            cout << endl;
            cout << "\n------------------------------------------------------------------------------------------------------------------------------" << endl;
    
            round++;
    
    }
    
    Block PoW::GenerateBlock (unsigned int nonce)
    {

        vector<uint8_t> prevHash;
        vector<Tx> vectTX = GetPendingTxs();

        if(round == 0)
        {
            cout << "노드 " << m_id << " : Add GenesisBlock" << endl;
            prevHash = network.GetGenesisPrevHash ();                       // ============== prevHash 설정 ============== //
        }
        else
        {
            prevHash = GetBlockHash (m_Blockchain.m_blocks.back ());        // ============== prevHash 설정 ============== //
        }


        //======= 블록 생성 =======
        Block newBlock (round, prevHash, network.difficulty, nonce, vectTX);

        return newBlock;
    }
    
    void PoW::SendBlock (vector<uint8_t> vect_block)
    {
        uint8_t *SerializedBlock = network.Bytes2Packet(vect_block);

        Ptr<Packet> p = Create<Packet> (SerializedBlock, vect_block.size());

        // cout << "\n vect_block.size() : "<< vect_block.size() << endl;
        // cout << "\n p->GetSize() : "<< p->GetSize() << endl;

        // uint8_t* buffer1 = new uint8_t[p->GetSize()];
        // p->CopyData(buffer1, p->GetSize());
        
        std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin ();
        
        while (iter != m_peersAddresses.end ())
        {
            Ptr<Socket> socketClient = m_peersSockets[*iter];
            Simulator::Schedule(Seconds(0), SendPacket, socketClient, p);
        
            iter++;
        }

        free(SerializedBlock);

        unsigned int nonce;
        nonce = dis (gen);
        int mineTime = 0;
        
        Simulator::Schedule (Seconds (1), &PoW::RunPoW, this, nonce, mineTime); // 채굴 시작
    }
    
    bool PoW::ValidationBlock (Block receive_block)
    {
    
        string str;
        string outputHash;
        str = to_string (receive_block.header.blockId) + network.Bytes2String(receive_block.header.prevHash) +
              to_string (receive_block.header.difficulty) + to_string(receive_block.header.nonce);
    
        // ============== 해시값 계산 ============== //
        outputHash = network.GetHash (str);
    
        // ============= 해시 출력값 앞자리 0 개수 체크 ======== //
        bool Check = network.CheckHash (outputHash) && VerifyTxs(receive_block);
    
        if (Check)
        {
            cout << "*** Node " << GetNode ()->GetId () << " : Valid block as a result of verification. ***" << endl;
            cout << "input_Hash : " << str << endl;
            cout << "output_Hash: " << outputHash << endl;
            // receive_block.PrintBlock();

            return true; // 검증 O
        }
        else
        {
            cout << "*** Node " << GetNode ()->GetId () << " : Invalid block as a result of verification. ***" << endl;
            cout << "input_Hash : " << str << endl;
            cout << "output_Hash: " << outputHash <<endl;

            receive_block.PrintBlock ();
    
            return false; // 검증 X
        }
    }

    // =============== Generate Transaction =============== //
    Tx PoW::GenerateTx(void) 
    {
        stringstream randomValue;
        Tx tx;
        int txSize = (dis(gen) % network.maxTxItemSize);
        // int txSize = 512;
        if (txSize % 2 == 1) { txSize += 1; }
        if (txSize == 0) { txSize += 2; }
        for (int i = 0; i < txSize; i++) 
        {
            randomValue << hex << dis(gen) % 16;
            tx.txItem.push_back(randomValue.str()[i]);
        }

        tx.sign = SignTx(tx.txItem, m_sk);
        tx.pk = m_pk;
        
        return tx;
    }

    vector<uint8_t> PoW::SignTx(vector<uint8_t> txItem, vector<uint8_t> sk) 
    {
        uint8_t signature[OQS_SIG_dilithium_2_length_signature] = {};
        size_t signature_len;

        uint8_t secret_key[OQS_SIG_dilithium_2_length_secret_key] = {};
        network.Bytes2Array(sk, secret_key);
        secret_key[OQS_SIG_dilithium_2_length_secret_key] = '\0';

        size_t message_len = txItem.size()/2;
        uint8_t message[message_len] = {};
        network.Bytes2Array(txItem, message);
        message[message_len] = '\0';

        OQS_SIG_dilithium_2_sign(signature, &signature_len, message, message_len, secret_key);
        signature[signature_len] = '\0';
        // cout << "[txItem]" << endl;
        // network.PrintBytes(txItem);
        // cout << "[message in SignTx]" << endl << message << endl;

        vector<uint8_t> sig = network.Array2Bytes(signature, 2420);
    
        return sig;
    }

    // =============== Get Pending Transactions from Memory Pool =============== //
    vector<Tx> PoW::GetPendingTxs(void) 
    {
        vector<Tx> newTxs;
        int txStorage = network.txStorage;

        while(true) {
            if (m_memPool.empty()) { 
                cout << "Memory Pool is empty" << endl; 
                break;
            }
            Tx tx = m_memPool.front();
            int txSize = tx.txItem.size() + tx.sign.size() + tx.pk.size();

            if (txSize <= txStorage) 
            {
                m_memPool.pop();
                newTxs.push_back(tx);
                txStorage = txStorage - txSize;
            }

            else { break; }
        }

       return newTxs;
    }

    bool PoW::VerifyTxs(Block block) 
    {
        OQS_STATUS rc;

        for (int j = 0; j < block.body.txs.size(); j++) 
        {
            Tx tx = block.body.txs[j];
            rc = VerifyTx(tx.txItem, tx.sign, tx.pk);
            if (rc != OQS_SUCCESS)
            {
                return false;
            } 
        }

        return true;
    }

    OQS_STATUS PoW::VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> pk) 
    {
        // cout << "here?" << endl;
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
        // if (rc != OQS_SUCCESS) 
        // { 
        //     cout << "VerifyTx Failed" << endl; 
        // }
        // else 
        // { 
        //     cout << "VerifyTx Successed" << endl; 
        // }

        return rc;
    }

} // namespace ns3}


