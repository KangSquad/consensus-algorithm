#ifndef PoW_NODE_H                                                     // 헤더 파일 중복 방지
#define PoW_NODE_H

#include <algorithm>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include <map>
#include <queue>
#include <oqs/oqs.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <stdio.h>

using namespace std;

namespace ns3 
{
    class Address;
    class Socket;
    class Packet;
    class Network;
    class Block;
    class Tx;

// ===================================================================================================================================================

    class Network 
    {
        public:
            // ===== 생성자 & 소멸자 ===== //
            Network(void);                                                                         
            virtual ~Network(void);                                                                

            // ===== 함수 ===== //
            char                        IntToChar (int num);                                        // Integer -> Character
            char                        CharToInt (char num);                                       // Character -> Integer
            
            vector<uint8_t>             GetGenesisPrevHash (void);                                  // 제네시스 블록의 이전블록의 해시값 받아오기 ("000 .... 000") (제네시스 블록은 이전 블록이 없기 때문에 별도의 함수를 통해 임의로 초기화)
            string                      GetHash(string input);                                      // 해시값 계산
            bool                        CheckHash(string output);                                   // nonce 값이 difficulty를 만족시키는 값인지 확인 (앞자리 0의 개수 확인)      
            int                         CountIntlen(int num);                                       // Integer의 자릿수 계산
            vector<uint8_t>             Array2Bytes(uint8_t arr[], int arrSize);                    // uint8_t [] -> vector<uint8_t>
            void                        Bytes2Array(vector<uint8_t> bytes, uint8_t * array);        // vector<uint8_t> -> uint8_t []
            string                      Bytes2String(vector<uint8_t> bytes);                        // vector<uint8_t> -> string
            uint8_t *                   Bytes2Packet(vector<uint8_t> vect);                         // 패킷화 vector<uint8_t> -> uint8_t [] (Bytes2Array와 거의 동일함)
            vector<uint8_t>             String2Bytes(string str);                                   // string -> vector<uint8_t>

            void                        PrintBytes(vector<uint8_t> vec);                            // vector <uint8_t> 출력

            // ===== 변수 ===== //
            int                         difficulty;                                                 // 블록 채굴 난이도
            int                         maxTxItemSize;                                              // 트랜잭션 길이 제한
            int                         blockSize;                                                  // 블록 크기
            int                         headerSize;                                                 // 블록의 헤더 크기
            int                         txStorage;                                                  // 블록에서 트랜잭션을 담을 수 있는 남은 공간

            clock_t start, end;                                                                     // 성능평가용

    };

// ===================================================================================================================================================

    class Block 
    {
        public:
            // ===== 생성자 & 소멸자 ===== //
            Block(void);                                                                                
            Block(int blockId, vector<uint8_t> prevHash, int difficulty, int nonce, vector <Tx> tx);    
            virtual ~Block(void);                                                                       

            class BlockHeader                                                                           // 블록 헤더
            {
                public:
                    BlockHeader(void);                                                                  
                    virtual ~BlockHeader(void);                                                         

                    // ===== 변수 ===== //
                    int             blockId;                                                            // 블록 ID (라운드와 동일)
                    int             difficulty;                                                         // 난이도
                    int             nonce;                                                              // nonce (채굴할 때 바꾸는 값)
                    vector<uint8_t> prevHash;                                                           // 이전 해시값
            };

            class BlockBody                                                                             // 블록 바디
            {
                public:
                    BlockBody(void);                                                                    
                    virtual ~BlockBody(void);                                                           

                    // ===== 변수 ===== //
                    vector <Tx>     txs;                                                                // 블록에 담긴 트랜잭션들

            };

            // ===== 함수 ===== //
            void            PrintBlock(void);                                                           // 블록 출력
            int             GetBlockSize(void);                                                         // 블록 크기
            string          GetBlockHash(void);                                                         // 블록의 해시값

            vector<uint8_t> SerializeBlock(void);                                                       // 블록을 벡터 형태로 변환
            vector<uint8_t> SerializeId(void);                                                          // Serialize BlockID
            vector<uint8_t> SerializePrevHash(void);                                                    // Serialize prevHash
            vector<uint8_t> SerializeDifficulty(void);                                                  // Serialize difficulty
            vector<uint8_t> SerializeNonce(void);                                                       // Serialize nonce
            vector<uint8_t> SerializeTxs(void);                                                         // Serialize txs
            void            DeserializeBlock(string packet);                                            // 블록 역직렬화
            

            // ===== 변수 ===== //
            BlockHeader     header;                                                                     // 블록 헤더
            BlockBody       body;                                                                       // 블록 바디
    };                             

// ===================================================================================================================================================

    class Blockchain 
    {                          
        public:      
            // ===== 생성자 & 소멸자 ===== //
            Blockchain(void);                                                                          
            virtual ~Blockchain(void);                                                                 

            // ===== 함수 ===== //
            void            AddBlock (Block newBlock);                                                  // 체인에 블록 추가
            void            PrintBlockchain(int round);                                                 // Round에 해당하는 블록 출력
            void            PrintChain(void);                                                           // 체인 출력         
            int             GetBlockChainHeight(void);                                                  // 체인의 길이 출력                            

            // ===== 변수 ===== //
            vector<Block>   m_blocks;                                                                   // 체인 내의 블록들
    };

// ===================================================================================================================================================

    class Tx {
        public:
            // ===== 생성자 & 소멸자 ===== //
            Tx(void);                                                                                 
            Tx(vector<uint8_t> newTx);                                                                
            virtual ~Tx(void);                                                                        

            // ===== 함수 ===== //
            void                                PrintTx(void);                                          // 트랜잭션 출력
            void                                GenerateTx(void);                                       // 트랜잭션 생성                 
            int                                 GetTxSize(void);                                        // 트랜잭션 크기 리턴

            // ===== 변수 ===== //
            vector<uint8_t>                     txItem;                                                 // 트랜잭션 내용 (시뮬레이션이기 때문에 임의로 초기화)
            vector<uint8_t>                     sign;                                                   // 서명값
            vector<uint8_t>                     pk;                                                     // 공개키
    };


// ===================================================================================================================================================

    class PoW : public Application 
    {
        public:
            // ===== 생성자 & 소멸자 ===== //
            PoW (void);                                                                                                            
            virtual ~PoW (void);                                                                                                   

            // ===== 함수 ===== //
            static TypeId                       GetTypeId (void);                                                                   // Get TypeId
            virtual void                        StartApplication (void);                                                            // Application class가 반드시 구현되어야 하는 가상 함수 상속
            virtual void                        StopApplication (void); 
            void                                SetPeersAddresses (const std::vector<Ipv4Address> &peers);                          // 모든 인접 노드의 주소 설정
            void                                HandleRead (Ptr<Socket> socket);                                                    // 메시지 받았을 떄 처리
            string                              GetPacketContent(Ptr<Packet> packet, Address from);                                 // 전송받은 패킷 내의 메시지를 string으로 파싱
            void                                SendBlock(vector<uint8_t> vect_block);                                              // 모든 인접 노드에게 블록 브로드캐스트
            void                                RunPoW (int nonce, int mineTime);                                                   // 합의 시작
            bool                                ValidationBlock(Block block);                                                       // 블록 검증
            Block                               GenerateBlock(unsigned int nonce);                                                  // 블록 생성
            string                              GetHashValue(string input);                                                         // 해시 계산
            vector<uint8_t>                     GetBlockHash(Block block);                                                          // 블록의 해시 계산
            void                                SuccessMine(int nonce, string inputHash, string outputHash);                        // 채굴 성공했을 때
            bool                                MiningBlock(string inputHash, int nonce, int mineTime);                             // 블록 채굴
            Tx                                  GenerateTx(void);                                                                   // Generate Transaction
            vector<uint8_t>                     SignTx(vector<uint8_t> txItem, vector<uint8_t> sk);                                 // 트랜잭션에 서명
            OQS_STATUS                          VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> pk);         // 트랜잭션 검증
            bool                                VerifyTxs(Block chain);                                                             // 트랜잭션들 검증
            void                                InitializeKey(void);                                                                // Initialize sign key
            void                                InitializeMempool(void);                                                            // Initialize Mempool
            vector<Tx>                          GetPendingTxs(void);                                                                // 트랜잭션 풀에서 트랜잭션 가져오기


            // ===== 변수 ===== //
            uint32_t                            m_id;                           // node id
            Ptr<Socket>                         m_socket;                       // socket
            map<Ipv4Address, Ptr<Socket>>       m_peersSockets;                 // 인접 노드의 소켓 목록
            map<Address, string>                m_bufferedData;                 // handleRead 이벤트에서 전송 받은 데이터를 보유하는 맵
            Address                             m_local;                        // 지금 노드의 주소
            vector<Ipv4Address>                 m_peersAddresses;               // 다른 노드들의 ip 주소들
            map<Ipv4Address, uint32_t>          m_peersIndex;                   // 다른 노드들의 index

            int                                 numNodes;                       // 총 노드 수
            int                                 round;                          // 합의 라운드
            Blockchain                          m_Blockchain;                   // 노드별 분산 원장 (블록체인)
            queue<Tx>                           m_memPool;                      // 메모리 풀

            vector<uint8_t>                     m_pk;                           // public key
            vector<uint8_t>                     m_sk;                           // secret key

    };
}
#endif







