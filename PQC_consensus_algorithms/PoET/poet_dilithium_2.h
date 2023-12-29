#ifndef PoET_NODE_H
#define PoET_NODE_H

#include <algorithm>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include <map>
#include <queue>
#include <vector>

#include <oqs/oqs.h>

using namespace std;

union
{
    unsigned char a[4];
    float b;
} union_data;

namespace ns3 {
    class Address;
    class Socket;
    class Packet;
    class Blockchain; 
    class Block;
    class Tx;
    class Network;
    class PoL;
    class TEE;


// ===================================================================================================================================================


    class Network {
        public:
            Network(void);
            ~Network(void);

            // ===== 함수 ===== //
            void                    PrintChain(string chainName, Blockchain chain);
            void                    PrintBlock(string blockName, Block block);
            void                    PrintVector(string vecName, vector<uint8_t> vec);
            void                    PrintIntVector(string vecName, vector<int> vec);
            void                    PrintTxs(string txName, vector<Tx> txs);
            uint8_t *               Block2Array(Block block);
            Block                   Array2Block(string packet);
            uint8_t *               Bytes2Packet(vector<uint8_t> vec, int vecSize);
            void                    Bytes2Array(vector<uint8_t> bytes, uint8_t * array);
            string                  Bytes2String(vector<uint8_t> bytes);
            void                    String2Array(string str, uint8_t *array);
            vector<uint8_t>         Array2Bytes(uint8_t arr[], int arrSize);



            float                   GetRandomDelay(void);                                       // 무작위 딜레이 값 계산 (Schedule의 Delay)

            // ===== 변수 ===== //
            int                 blockSize;
            int                 prevHashSize;
            int                 txStorage;

            int                 minimumWait;
            float               localAverage;
            float               zMax;
            int                 fastestNode;
            
            vector<int>         winNum;
            int                 numNodes;
            int                 numTxs;
            int                 maxTxItemSize;

    };


// ===================================================================================================================================================


    class Blockchain {
        public:
            Blockchain(void);
            ~Blockchain(void);

            // ===== 함수 ===== //
            void                    AddBlock(Block newBlock);
            int                     GetBlockchainHeight(void);
            Block                   GetLatestBlock(void);

            // ===== 변수 ===== //
            vector<Block>           blocks;
    };


// ===================================================================================================================================================


    class Block {
        public:
            Block(void);
            Block(vector<uint8_t> prevHash, vector<Tx> txs);
            ~Block(void);
            
            class BlockHeader {
                public:
                    BlockHeader(void);
                    ~BlockHeader(void);
                    
                    // ===== 변수 ===== //
                    vector<uint8_t>     prevHash;
            };

            class BlockBody {
                public:
                    BlockBody(void);
                    ~BlockBody(void);
                    
                    // ===== 변수 ===== //
                    vector<Tx>     txs;
            };
            
            // ===== 함수 ===== //
            void SetHeader(vector<uint8_t> prevHash);
            void SetBody(vector<Tx> txs);

            pair<vector<uint8_t>, int>      SerializeBlock(void);
            pair<vector<uint8_t>, int>      SerializePrevHash(void);
            pair<vector<uint8_t>, int>      SerializeTxs(void);
            void                            DeserializeBlock(string packet);


            // ===== 변수 ===== //
            BlockHeader     header;
            BlockBody       body;
    };


// ===================================================================================================================================================


    class Tx {
        public:
            Tx(void);
            Tx(vector<uint8_t> newTx);
            virtual ~Tx(void);

            void                            PrintTx(void);
            pair<vector<uint8_t>, int>      SerializeTx(void);
            void                            DeserializeTx(string packet);
            int                             GetTxSize(void);

            vector<uint8_t>                 txItem;
            vector<uint8_t>                 sign;
            vector<uint8_t>                 pk;
    };


// ===================================================================================================================================================


    class TEE {
        public:
            TEE(void);
            ~TEE(void);

            // ===== 함수 ===== //

            // ===== 변수 ===== //
    };


// ===================================================================================================================================================


    class PoET : public Application {
        public:
            PoET(void);
            ~PoET(void);

            // ===== 함수 ===== //
            static TypeId       GetTypeId(void);                                                // TypeId 설정

            virtual void        StartApplication(void);                                         //
            virtual void        StopApplication(void);                                          //
            void                HandleRead(Ptr<Socket> socket);                                 //
            void                SendPacket(Ptr<Socket> socketClient, Ptr<Packet> p);            //
            void                SendBlock(Block block);                                         //
            string              GetPacketContent(Ptr<Packet> packet, Address from);             //

            vector<uint8_t>     GetBlockHash(Block block);
            void                AddPendingTxs(Tx pendingTx);
            vector<Tx>          GetPendingTxs(void);
            float               GetWaitTime(void);

            void                NewRound(void);
            bool                ZTest(void);

            void                InitializeChain(void);
            void                InitializeMempool(void);
            void                InitializeKey(void);
            Tx                  GenerateGenesisTx(void);
            Tx                  GenerateTx(void);

            vector<uint8_t>     SignTx(vector<uint8_t> txItem, vector<uint8_t> sk);
            bool                VerifyTxs(Block block);
            bool                VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> pk);

            // ===== 변수 ===== //
            uint32_t                            m_id;                                           //
            Address                             m_local;                                        //
            Ptr<Socket>                         m_socket;                                       //
            Ptr<Socket>                         m_socketClient;                                 //
            vector<Ipv4Address>                 m_peersAddresses;                               //
            map<Ipv4Address, Ptr<Socket>>       m_peersSockets;                                 //
            map<Ptr<Socket>, int>               m_peersNodeId;
            map<Address, string>                m_bufferedData;                                 //
            EventId                             m_eventId;

            TEE                                 m_secureWorker;
            queue<Tx>                           m_memPool;

            float                               m_waitTime;
            Blockchain                          m_chain;
           
            vector<uint8_t> m_pk;
            vector<uint8_t> m_sk;            

    };
}
#endif