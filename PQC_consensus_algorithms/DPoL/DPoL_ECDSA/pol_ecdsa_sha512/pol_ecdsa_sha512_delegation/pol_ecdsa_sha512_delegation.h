#ifndef PoL_NODE_H
#define PoL_NODE_H

#include <algorithm>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include <map>
#include <queue>
#include <random>

#include <oqs/oqs.h>

#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/ecdsa.h>
#include <openssl/rand.h> 
#include <openssl/bn.h> 
#include <stdio.h>

using namespace std;

union {
    unsigned char   c[4];
    float     f;
} union_data;

inline static constexpr const uint8_t Base58Map[] = {
    '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q',
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z' };

inline static constexpr const uint8_t AlphaMap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0xff, 0x11, 0x12, 0x13, 0x14, 0x15, 0xff,
    0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0xff, 0x2c, 0x2d, 0x2e,
    0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xff, 0xff, 0xff, 0xff, 0xff };

using CodecMapping = struct _codecmapping {
    _codecmapping(const uint8_t* amap, const uint8_t* bmap) : AlphaMapping(amap), BaseMapping(bmap) {}
    const uint8_t* AlphaMapping;
    const uint8_t* BaseMapping;
};

namespace ns3 {
    class Address;
    class Socket;
    class Packet;
    class Network;
    class TEE;
    class Blockchain;
    class Block;
    class Tx;
    class PoL;    

    // void cleanup_stack(uint8_t *secret_key, size_t secret_key_len);
    // void cleanup_heap(uint8_t *public_key, uint8_t *secret_key, uint8_t *message, uint8_t *signature, OQS_SIG *sig);
    // static OQS_STATUS example_stack(void);
    // static OQS_STATUS example_heap(void);

// ===================================================================================================================================================


    class Network {
        public:
            Network(void);
            virtual ~Network(void);

            // ===== 함수 ===== //
            void                            PrintBytes(vector<uint8_t> vec);
            string                          Bytes2String(vector<uint8_t> bytes);
            vector<uint8_t>                 String2Bytes(string str);
            void                            String2Array(string str, uint8_t *array);
            float                           Hex2Float(string hex);
            uint8_t *                       Bytes2Packet(vector<uint8_t> vec, int vecSize);
            vector<uint8_t>                 Array2Bytes(uint8_t arr[], int arrSize);
            void                            Bytes2Array(vector<uint8_t> bytes, uint8_t * array);
            // void                            Bytes2UChar(vector<uint8_t> bytes, unsigned char* val);     
            // vector<uint8_t>                 UChar2Bytes(unsigned char * val, int valSize);                       
            string                          Eckey2String(EC_KEY * ecKey);
            string                          Sig2String(ECDSA_SIG * sig);
            EC_KEY *                        String2Eckey(string str, EC_KEY * eckey);
            ECDSA_SIG *                     String2Sig(string str, ECDSA_SIG * sig);
            string                          Base58Encode(const vector<uint8_t> &data);
            vector<uint8_t>                 Base58Decode(const string &data);
            
            float                           GetRandomDelay(void);

            // ===== 변수 ===== //
            int                             blockSize;  
            int                             headerSize;
            int                             bodySize;
            int                             prevHashSize;
            int                             txStorage;
            int                             maxTxItemSize;
            
            double                          signTime;
            double                          verifyTime;
            double                          keyGenTime;
            
            int                             numTx;
            int                             numSign;
            
            clock_t                         start;
            clock_t                         end;
    
            int                             numNodes;
            vector<float>                   vote;

            int                             numDelegates;
            vector<int>                     delegated;     
    };


// ===================================================================================================================================================


    class Blockchain {
        public:
            Blockchain(void);                                                                                               // Blockchain 생성자
            virtual ~Blockchain(void);                                                                                      // Blockchain 소멸자

            // ===== 함수 ===== //
            void                            AddBlock(Block newBlock);                                                       // 체인에 블록을 추가하는 함수
            int                             GetBlockchainHeight(void);                                                      // 현재 체인의 길이를 계산하는 함수
            Block                           GetLatestBlock(void);                                                           // 최신 블록을 리턴하는 함수
            
            void                            PrintChain(void);
            pair<vector<uint8_t>, int>      SerializeChain(void);
            void                            DeserializeChain(string packet);

            // ===== 변수 ===== //
            vector<Block>                   blocks;                                                                         // 분산원장
    };
    

// ===================================================================================================================================================


    class Block {
        public:
            Block(void);                                                                                                    // Block 기본 생성자
            Block(vector<uint8_t> prevHash, vector<Tx> txs, vector<uint8_t> proof);                                         // Block 생성자
            virtual ~Block(void);                                                                                           // Block 소멸자

            class BlockHeader {
                public:
                    BlockHeader(void);                                                                                      // BlockHeader 생성자
                    virtual ~BlockHeader(void);                                                                             // BlockHeader 소멸자

                    // ===== 함수 ===== //
                    vector<uint8_t>         GetHeaderHash();                                                                //

                    // ===== 변수 ===== //
                    vector<uint8_t>         prevHash;                                                                       // 이전 블록의 해시값
                    vector<Tx>              txs;                                                                            // 트랜잭션 (16진수)
            };

            class BlockBody {
                public:
                    BlockBody(void);                                                                                        // BlockBody 생성자
                    virtual ~BlockBody(void);                                                                               // BlockBody 소멸자

                    // ===== 변수 ===== //
                    vector<uint8_t>         proof;                                                                          // PoL로부터 생성된 값
            };

            // ===== 함수 ===== //
            void                            SetHeader(vector<uint8_t> prevHash, vector<Tx> txs);                            // BlockHeader 설정
            void                            SetBody(vector<uint8_t> proof);                                                 // BlockBody 설정

            void                            PrintBlock(void);
            pair<vector<uint8_t>, int>      SerializeBlock(void);
            pair<vector<uint8_t>, int>      SerializePrevHash(void);
            pair<vector<uint8_t>, int>      SerializeProof(void);
            pair<vector<uint8_t>, int>      SerializeTxs(void);
            void                            DeserializeBlock(string packet);

            int                             GetBlockSize(void);
            vector<uint8_t>                 GetBlockHash(void);                                                             // 블록의 해시값을 계산하는 함수

            // ===== 변수 ===== //
            BlockHeader                     header;                                                                         // BlockHeader
            BlockBody                       body;                                                                           // BlockBody
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

            vector<uint8_t>                 txItem;
            vector<uint8_t>                 signature;
            vector<uint8_t>                 ecKey;
    };


// ===================================================================================================================================================


    class TEE {
        public:
            TEE(void);                                                                                                      // TEE 생성자
            virtual ~TEE(void);                                                                                             // TEE 소멸자

            // ===== 함수 ===== //
            vector<uint8_t>                 RemoteAttestation(vector<uint8_t> nonce, float luck);                           // TEE를 통한 Remote Attestation 
            Time                            GetTrustedTime(void);                                                           // TEE를 통해 현재 시간 Get
            void                            IncrementMonotonicCounter(void);                                                // Monotonic Counter 값 증가
            int                             ReadMonotonicCounter(void);                                                     // Monotonic Counter 값 Get
            float                           LuckInProof(vector<uint8_t> proof);                                             // proof를 통해 luck값 계산
            vector<uint8_t>                 NonceInProof(vector<uint8_t> proof);                                            // proof를 통해 nonce값 계산

            // ===== 변수 ===== //
            int                             m_counter;                                                                      // Monotonic Counter
    };


// ===================================================================================================================================================


    class PoLNode : public Application {
        public :
            // ===== 함수 ===== //
            PoLNode(void);                                                                                                  // PoL 
            virtual ~PoLNode(void);                                                                                         //

            static TypeId                   GetTypeId(void);                                                                // TypeId 설정

            virtual void                    StartApplication(void);                                                         //
            virtual void                    StopApplication(void);                                                          //
            void                            HandleRead(Ptr<Socket> socket);                                                 //
            void                            SendPacket(Ptr<Socket> socketClient, Ptr<Packet> p);                            //
            void                            SendTx(Tx tx);                                                                  //
        //  void                            SendBlock(Block block);                                                         //
            void                            SendChain(Blockchain chain);                                                    //
            string                          GetPacketContent(Ptr<Packet> packet, Address from);                             //

            vector<Tx>                      GetPendingTxs(void);                                                            //
            void                            AddPendingTx(Tx pendingTx);                                                     //    
            Tx                              GenerateGenesisTx(void);                                                        //
            Tx                              GenerateTx(void);                                                               //
            string                          SignTx(vector<uint8_t> txItem, EC_KEY * ec_key);
            bool                            VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> ecKey);
            bool                            VerifyTxs(Blockchain chain);

            void                            InitializeKey(void);
            void                            InitializeChain(void);
            void                            InitializeMempool(void);
            float                           GetRandomLuck(void);                                                            //
            float                           GetWaitingTime(float luck);                                                     //
            void                            NewRound(void);                                                                 //
            void                            PoLRound(Block roundBlock);                                                     //
            void                            PoLMine(Block::BlockHeader header, Block previousBlock);                        //
            void                            Commit(vector<Tx> newTxs);                                                      //
            void                            Valid(Blockchain chain);                                                        //
            float                           Luck(Blockchain chain);                                                         //
            void                            Vote(void);
            void                            SelectDelegates(void);
            bool                            isDelegated(int idx);

            // ===== 변수 ===== //
            int                             round;                                                                          //
            
            uint32_t                        m_id;                                                                           //
            Address                         m_local;                                                                        //
            Ptr<Socket>                     m_socket;                                                                       //
            Ptr<Socket>                     m_socketClient;                                                                 //
            vector<Ipv4Address>             m_peersAddresses;                                                               //
            map<Ipv4Address, Ptr<Socket>>   m_peersSockets;                                                                 //
            map<Address, string>            m_bufferedData;                                                                 //

            Blockchain                      m_chain;                                                                        //
            Block                           m_roundBlock;                                                                   //
            Time                            m_roundTime;                                                                    //
            TEE                             m_secureWorker;                                                                 // 
            queue<Tx>                       m_memPool;                                                                      // memory pool

            EC_KEY *                        m_ecKey;
            BIGNUM *                        m_privateKey;
            const EC_GROUP *                m_group;
            EC_POINT *                      m_publicKey;
            BN_CTX*                         m_ctx;
    };
}
#endif