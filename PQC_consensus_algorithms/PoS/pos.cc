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
#include "pos.h"
#include <algorithm>
#include "stdlib.h"
#include "ns3/ipv4.h"
#include "SHA256.h"
#include <random>
#include <stdlib.h>
#include <ctime>
#include <unistd.h>
#include <map>
#include <iomanip>


using namespace std;


int t_id;
int node_size = 4;
// int total_coinAges[5]; // 코인 나이 저장소
int error_make[1]={1};


random_device rd;
mt19937 gen (rd ());
uniform_int_distribution<int> dis (0, 99);

namespace ns3 
{

    static Network network;

    NS_LOG_COMPONENT_DEFINE ("PoS");
    NS_OBJECT_ENSURE_REGISTERED (PoS);

    // 네트워크
    Network::Network (void)
    {
        blockSize = 20480 * 1.5;                    // 20480 = 10 KB
        headerSize = 72;                            // 72 bytes 

        txStorage = blockSize - headerSize;         
        maxTxItemSize = 512;        

    }

    Network::~Network (void)
    {
    }

    char Network::IntToChar (int num)
    {
        return num + '0';
    }

    char Network::CharToInt (char num)
    {
        return num - '0';
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

    // =============== Convert Bytes(vector) to Packet(array) =============== //  
    uint8_t * Network::Bytes2Packet(vector<uint8_t> vect) {
        int vecSize = vect.size();
        uint8_t * packet = (uint8_t *)malloc(vecSize+1 * sizeof(uint8_t));
        for (int i = 0; i < vecSize; i++) { packet[i] = vect[i]; }
        packet[vecSize] = '\0';

        return packet;
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

    // 패킷 브로드캐스트
    void SendPacket (Ptr<Socket> socketClient, Ptr<Packet> p)
    {
        socketClient->Send (p);
    }

    // =============== Tx Constructor =============== //
    Tx::Tx(void) 
    {
    }

    // =============== Tx Constructor =============== //
    Tx::Tx(vector<uint8_t> txItem) 
    {
        this->txItem = txItem;
    }

    // =============== Tx Destructor =============== //
    Tx::~Tx(void) 
    {
        NS_LOG_FUNCTION(this);
    }

    // 블록체인
    Blockchain::Blockchain (void)
    {
    }

    Blockchain::~Blockchain (void)
    {
    }

    void Blockchain::AddBlock (Block newBlock)
    {
        m_blocks.push_back(newBlock);
    }

    void Blockchain::printBlockchain(int idx)
    {
        cout << "Block " << idx <<endl;
        cout << "{" << endl << "   blockId : " << this -> m_blocks[idx].header.blockId <<endl;
        cout << "   PrevHash : ";
        for(int i=0; i < this->m_blocks[idx].header.prevHash.size(); i++)
        {
            cout <<  this->m_blocks[idx].header.prevHash[i];
        }
        cout << endl;
        cout << "   CoinAge : " << this->m_blocks[idx].header.coinAge << endl;

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
            cout << "   }" <<endl;
        }

        cout << "}" << endl; 
    }

    // 블록
    Block::Block(void)
    {
    }

    Block::~Block(void)
    {
    }

    // =============== Block Class Constructor =============== //
    Block::Block (int blockId, vector<uint8_t> prevHash, int coinAge, vector<Tx> tx)
    {
        this->header.blockId = blockId;
        // this->header.prevHash = prevHash;
        this->header.prevHash.resize(prevHash.size());
        copy(prevHash.begin(), prevHash.end(), this->header.prevHash.begin());

        this->header.coinAge = coinAge;

        for (int i=0; i < tx.size(); i++)
        {
            this -> body.txs.push_back(tx[i]);
        }

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

    // =============== Serialize Block =============== //
    vector<uint8_t> Block::SerializeBlock (void)
    {
    
        vector<uint8_t> serializeId = this->SerializeId();
        vector<uint8_t> serializePrevhash = this->SerializePrevHash();
        vector<uint8_t> serializeCoinAge = this->SerializeCoinAge();
        vector<uint8_t> serializeTxs = this->SerializeTxs();

        vector<uint8_t> serializedBlock;

        serializedBlock.push_back('/');
        serializedBlock.push_back('b');    // 'b'lock

        for (int i=0; i< serializeId.size();i++)            {  serializedBlock.push_back(serializeId[i]);}
        for (int i=0; i< serializePrevhash.size();i++)      {  serializedBlock.push_back(serializePrevhash[i]);}
        for (int i=0; i< serializeCoinAge.size();i++)       {  serializedBlock.push_back(serializeCoinAge[i]);}
        for (int i=0; i< serializeTxs.size();i++)           {  serializedBlock.push_back(serializeTxs[i]);}

        return serializedBlock;
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

    vector<uint8_t> Block::SerializeCoinAge(void)                                                  // Serialize coinAge
    {
        vector<uint8_t> SerializedcoinAge;

        int coinAge = this->header.coinAge;
        int coinAge_len = network.CountIntlen (coinAge);

        for (int idx = 0; idx < coinAge_len; idx++)
        {
            SerializedcoinAge.push_back(network.IntToChar (coinAge % 10));
            coinAge = coinAge / 10;
        }

        SerializedcoinAge.push_back('a'); // coin'a'ge
        SerializedcoinAge.push_back('/');

        reverse(SerializedcoinAge.begin(), SerializedcoinAge.end());

        return SerializedcoinAge;
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
                else if (packet[i + 1] == 'a')  { offset = 'a'; i++;                                               continue; } // coin'a'ge
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

                // coin'a'ge
                case 'a':
                {
                    string str_coinAge="";
                    while(packet[i]!='/')
                    {
                        str_coinAge += packet[i];
                        i++;
                    }
                    this->header.coinAge = stoi(str_coinAge);
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

        cout << "   CoinAge : " << this -> header.coinAge << endl;

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

    // 노드
    PoS::PoS (void)
    {
    }

    PoS::~PoS (void)
    {
        NS_LOG_FUNCTION (this);
    }

    vector<uint8_t> PoS::GetBlockHash (Block block)
    {
        string input = "";
        string output;
    
        input += to_string (block.header.blockId);
        input += network.Bytes2String(block.header.prevHash);
        input += to_string(block.header.coinAge);
    
        output = network.GetHash (input);

        return network.String2Bytes(output);
    
        // return output;
    }

    TypeId PoS::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::PoS").SetParent<Application> ().SetGroupName ("Applications").AddConstructor<PoS> ();

        return tid;
    }

    vector<uint8_t> PoS::SignTx(vector<uint8_t> txItem, vector<uint8_t> sk) 
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
    vector<Tx> PoS::GetPendingTxs(void) 
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

    // =============== Generate Transaction =============== //
    Tx PoS::GenerateTx(void) 
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

    // =============== Generate Block =============== //
    Block PoS::generateBlock (unsigned int age)
    {
        cout << endl << "======================================================= round : " << round << " =======================================================" << endl;

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
        Block newBlock (round, prevHash, age, vectTX);

        return newBlock;

    }

    // =============== Initialize Key Pair =============== //
    void PoS::InitializeKey(void) 
    {
        uint8_t public_key[OQS_SIG_dilithium_2_length_public_key];
        uint8_t secret_key[OQS_SIG_dilithium_2_length_secret_key];

        OQS_STATUS rc = OQS_SIG_dilithium_2_keypair(public_key, secret_key);
        m_pk = network.Array2Bytes(public_key, OQS_SIG_dilithium_2_length_public_key);
        m_sk = network.Array2Bytes(secret_key, OQS_SIG_dilithium_2_length_secret_key);
    }

    // =============== Initialize Memory Pool =============== //
    void PoS::InitializeMempool(void) 
    {
        for (int i = 0; i < 500; i++) 
        {
            Tx tx = GenerateTx();
            m_memPool.push(tx);
        }
    }

    void PoS::StartApplication ()
    {

        // ===== Initializing ===== //
        InitializeKey();
        InitializeMempool();

        //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
        //NS_LOG_INFO("Node ID : "<<GetNode()->GetId());

        // 전역변수 초기화
        m_id = GetNode ()->GetId ();
        cout << "Node id : " << m_id << endl;

        round = 0;

        network.stakingVolume.push_back(-1);
        m_stakingIdx = network.stakingVolume.size() - 1;


        if (!m_socket)                                                                              // 지금 노드에 socket이 없다면, socket 설정                                                                      
        {   
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");                            // TypeId 설정 
    
            m_socket = Socket::CreateSocket (GetNode (), tid);                                      // 해당 TypeId로 socket 생성
    
            InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 7071);             // IP 주소와 포트 번호(7071)로부터 소켓의 주소를 만든다
    
            m_socket->Bind (local);                                                                 // 소켓에 ip와 포트(7071)를 할당
    
            m_socket->Listen ();                                                                    // 대기열에서 대기
        }   
    
        m_socket->SetRecvCallback (MakeCallback (&PoS::HandleRead, this));                          // 새로운 데이터를 읽을 수 있을 때 알려주는 역할
    
        m_socket->SetAllowBroadcast (true);                                                         // 브로드캐스트가 가능한지 설정
    
        std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin ();                        // 모든 인접 노드의 주소를 불러오기 위한 Iterator
            
        while (iter != m_peersAddresses.end ())                                                     // 모든 인접 노드와 연결
        {           
                
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");                            // TypeId 설정
    
            Ptr<Socket> socketClient = Socket::CreateSocket (GetNode (), tid);                      // 해당 TypeId로 client socket 생성
    
            socketClient->Connect (InetSocketAddress (*iter, 7071));                                // *iter에 해당하는 노드와 연결
    
            m_peersSockets[*iter] = socketClient;                                                   // m_peersSockets에 socketClient 저장

            iter++;
        }

        Simulator::Schedule (Seconds (1), &PoS::CoinAllocation, this);
    }

    void PoS::StopApplication ()
    {
    }

    void PoS::CoinAllocation(void)
    {
        cout << endl;

        coin_volume = dis(gen) + 1;                                                                 // 1부터 100사이의 코인을 할당 받음
        cout << "Node"<<m_id<<" : Number of coins allocated : "<<coin_volume << endl;
        
        double coin_ratio = dis(gen) + 1;                                                           // 스테이킹할 비율

        staking_Volume = coin_volume * coin_ratio / 100;                                            // 스테이킹할 코인 개수

        coin_volume = coin_volume - staking_Volume;                                                 // 보유한 코인에서 스테이킹한 코인 빼기

        staking_day = 1;                                                                            // 스테이킹 일수

        cout << "Node"<<m_id<<" : Number of coins staked : "<<staking_Volume << endl;

        CoinageCalc();
    }

    void PoS::CoinageCalc (void)
    {

        // 코인 나이 계산 (스테이킹한 코인 * 스테이킹 일수)
        intCoinAge = staking_Volume * staking_day;
        string str_Coin = to_string (intCoinAge);

        vector<uint8_t> VectcoinAge = network.String2Bytes(str_Coin);
        VectcoinAge.insert(VectcoinAge.begin(), 'c');
        VectcoinAge.insert(VectcoinAge.begin(), '/');

        network.stakingVolume[m_stakingIdx] = intCoinAge;

        /*
        int str_Coin_size = str_Coin.length ();

        uint8_t *Coin_age = (uint8_t *) malloc (str_Coin_size * sizeof (uint8_t));

        int i;
        for (i = 0; i < str_Coin_size; i++)
        {
          Coin_age[i] = str_Coin[i];
        }

        Coin_age[i] = '\0';

        staking_values[0] = intCoinAge;                                                            // 각 노드들의 스테이킹 값을 저장하는 배열
        */
        SendData(VectcoinAge);
        cout << "Node" << m_id << " : Coin age(" << intCoinAge << ") broadcast." << endl;

        // free (Coin_age);
    }

    string PoS::GetPacketContent (Ptr<Packet> packet, Address from)                                 // 패킷 수신
    {
        char *packetInfo = new char[packet->GetSize () + 1];

        std::ostringstream totalStream;

        packet->CopyData (reinterpret_cast<uint8_t *> (packetInfo), packet->GetSize ());

        packetInfo[packet->GetSize ()] = '\0';

        totalStream << m_bufferedData[from] << packetInfo;

        std::string totalReceivedData (totalStream.str ());

        return totalReceivedData;
    }

    void PoS::HandleRead (Ptr<Socket> socket)
    {

        Ptr<Packet> packet;
        Address from;

        string msg;

        // RecvFrom을 통해 소켓 내의 데이터 수신
        while ((packet = socket->RecvFrom (from)))
        {
            socket->SendTo (packet, 0, from);                                                       // 특정 인접 노드에게 데이터 전송

            if (packet->GetSize () == 0)                                                            // if EOF라면
            {
                break;
            }
            
            if (InetSocketAddress::IsMatchingType (from))                                           // from이 Address라면 1, 나머지는 0
            {
                msg = GetPacketContent (packet, from);

                if (msg[1]== 'c')                                                              // 코인 나이를 받은 경우
                {
                    auto it = find(network.stakingVolume.begin(), network.stakingVolume.end(), -1);

                    if(it== network.stakingVolume.end())    // -1이 없을 경우
                    {
                        int maxValue = *max_element(network.stakingVolume.begin(), network.stakingVolume.end());
                        int maxIndex = find(network.stakingVolume.begin(), network.stakingVolume.end(), maxValue) - network.stakingVolume.begin();

                        if (maxIndex == m_stakingIdx)       // 블록 생성자가 된 경우
                        {
                            // sleep(1);
                            Block m_block = generateBlock(maxValue);
                            m_Blockchain.AddBlock(m_block);                                                 // 체인에 블록 입력

                            vector<uint8_t> vect_block = m_block.SerializeBlock ();                         // 클래스로 구현된 블록을 arr로 변환

                            SendData(vect_block);                                                           //블록 전송
                            // free(arr_block);

                            cout << endl;
                            cout << "<Block genertation>\n";
                            cout << "The node with the oldest coin age creates the block.\n";
                            cout << "Node"<< m_id << " : Send block after generating block.\n";
                            cout << "Node"<< m_id << " blocks sent (round "<< round << ")\n";
                            m_Blockchain.printBlockchain(round);
                            cout << endl;

                            staking_day = 0;                                                                // 블록생성자는 스테이킹 시간 0으로 초기화
                            coin_volume += 5;                                                               // 블록 생성 보상
                            round++;                                                                        // 라운드 +1

                            fill(network.stakingVolume.begin(),network.stakingVolume.end(), -1);

                            Simulator::Schedule (Seconds (1), &PoS::CoinageCalc, this);                     // 다시 코인 스테이킹

                        }
                    }
                }
                else if (msg[1]=='b')                                                                               // 블록을 받은 경우
                {

                    Block receivedBlock;
                    receivedBlock.DeserializeBlock (msg);

                    bool check = ValidationBlock(receivedBlock);

                    cout << "<Block reception>\n";
                    cout << "round : " << round << endl;

                    if(check)                                                                       // 블록 검증값에 따라 블록체인에 추가하거나 무시하거나
                    {
                        cout << "Verification result: valid block" << endl;
                        cout << "Node" << m_id << " : The received block has been added to the chain.\n" << endl;

                        m_Blockchain.AddBlock(receivedBlock);
                        // m_Blockchain.printBlockchain(round);

                        round ++;
                        staking_day += 1;                                                           // 라운드가 지남에 따라 staking_day + 1
                    }
                    else 
                    {
                        cout << "Verification result: invalid block" << endl;
                        cout << "Node" << m_id << " : The received block was not added to the chain." << endl;
                        cout << "block received {" << endl;
                        receivedBlock.PrintBlock();
                        cout << "}"<< endl;

                        return;
                    }
                    Simulator::Schedule (Seconds (1), &PoS::CoinageCalc, this);
                }
                else
                {
                    cout << "error" << endl;
                }
            }
        }
    }

    void PoS::SendData (vector<uint8_t> vect_data)
    {
        uint8_t *SerializedBlock = network.Bytes2Packet(vect_data);

        Ptr<Packet> p = Create<Packet> (SerializedBlock, vect_data.size());

        std::vector<Ipv4Address>::iterator iter = m_peersAddresses.begin ();

        while (iter != m_peersAddresses.end ())
        {
            Ptr<Socket> socketClient = m_peersSockets[*iter];
            Simulator::Schedule (Seconds (0), SendPacket, socketClient, p);

            iter++;
        }

        free(SerializedBlock);

    }

    bool PoS::ValidationBlock (Block receive_block)
    {
        string str="";
        string prevHash ="";

        if(m_Blockchain.m_blocks.size() == 0)
        {
            vector <uint8_t> vectprevHash = network.GetGenesisPrevHash();
            prevHash = network.Bytes2String(vectprevHash);
        }
        else 
        {
            vector<uint8_t> vectprevHash = GetBlockHash(m_Blockchain.m_blocks.back());
            prevHash = network.Bytes2String(vectprevHash);
        }

        bool check_prevHash = prevHash == network.Bytes2String(receive_block.header.prevHash);
        bool check_tx = VerifyTxs(receive_block);

        if (check_prevHash && check_tx)
        {
            return true;
        }
        else 
        {
            cout << "이전 블록의 헤더 연접한 값 : " << str << endl;
            cout << "이전 블록의 헤더를 연접하여 해시한 값 : " << prevHash << endl;
            cout << endl;

            return false;
        }
    }

    bool PoS::VerifyTxs(Block block) 
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

    OQS_STATUS PoS::VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> pk) 
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

} // namespace ns3
