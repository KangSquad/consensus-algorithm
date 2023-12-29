#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
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
#include "pol_ecdsa_sha512_delegation.h"
#include <stdbool.h>
#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "ns3/ipv4.h"
#include "SHA.h"
#include "SHA512.h"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <string>

#define ROUND_TIME 0
#define MESSAGE_LEN 50

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> dis (1, 9999);

// TODO: 실제 tx이 들어온다고 가정했을 때, GenerateTx 전체 수정
// TODO: 이상한 값 출력되는 애들을 0 - 7F 까지만 입력되도록 하면 됨. (& 0x7F 해도 되지 않나?) --> 근데 signature 같은 건 우리가 입력한 게 아니라서 문제임

using namespace std;

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("PoLNode");                             // set NS_LOG macro's name
    NS_OBJECT_ENSURE_REGISTERED(PoLNode);                           // register PoLNode object to TypeId system

    static Network network;

// ===================================================================================================================================================


    void cleanup_stack(uint8_t *secret_key, size_t secret_key_len) {
        OQS_MEM_cleanse(secret_key, secret_key_len);
    }


    void cleanup_heap(uint8_t *public_key, uint8_t *secret_key, uint8_t *message, uint8_t *signature, OQS_SIG *sig) {
        if (sig != NULL) {
            OQS_MEM_secure_free(secret_key, sig->length_secret_key);
        }

        OQS_MEM_insecure_free(public_key);
        OQS_MEM_insecure_free(message);
        OQS_MEM_insecure_free(signature);
        OQS_SIG_free(sig);
    }

    // =============== Network Class Constructor =============== //
    Network::Network(void) {

        blockSize = 20480 * 3;         
        bodySize = 2048;                                // 1 KB
        headerSize = blockSize - bodySize;              //   MB
        prevHashSize = 128;                          // 256  bits (SHA-256)
        
        txStorage = headerSize - prevHashSize;      // 
        maxTxItemSize = 512;                        // 256 bytes

        signTime = 0;
        verifyTime = 0;
        keyGenTime = 0;

        numTx = 100;
        numSign = 0;

        numNodes = 8;
        numDelegates = 2;

    }

    // =============== Network Class Destructor =============== //
    Network::~Network(void) {
        NS_LOG_FUNCTION(this);
    }

    // =============== Convert string to bytes =============== //
    vector<uint8_t>
    Network::String2Bytes(string str) {
        vector<uint8_t> vec;
        for (size_t i = 0; i < str.size(); i++) { vec.push_back(str[i]); }

        return vec;
    }

    // =============== Convert bytes to string =============== //
    string
    Network::Bytes2String(vector<uint8_t> vec) {
        string str = "";
        for (size_t i = 0; i < vec.size(); i++) { str += vec[i]; }

        return str;
    }

    // =============== Convert Hex to Float (union) =============== //
    float
    Network::Hex2Float(string hex) {
        const char *hexString;
        int number;

        for (int i = 0; i < 4; i++) {
            hexString = hex.substr(i * 2, 2).c_str();
            number = (int)strtol(hexString, NULL, 16);
            union_data.c[i] = number;
        }

        return union_data.f;
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

    // =============== Convert Bytes(vector) to Packet(array) =============== //  
    uint8_t *
    Network::Bytes2Packet(vector<uint8_t> vec, int vecSize) {
        uint8_t * packet = (uint8_t *)malloc((vecSize + 1) * sizeof(uint8_t));
        for (int i = 0; i < vecSize; i++) { packet[i] = vec[i]; }
        packet[vecSize] = '\0';

        return packet;
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

    string
    Network::Eckey2String(EC_KEY * ecKey) {
        stringstream str;
        str << ecKey;

        return str.str();
    }

    EC_KEY *
    Network::String2Eckey(string str, EC_KEY* eckey) {
        cout << "eckey in String2Eckey: " << str << endl;
        uintptr_t temp;
        stringstream ss;
        ss << str;
        ss >> hex >> temp;

        eckey = reinterpret_cast<EC_KEY *> (temp);
        cout << "eckey after" << eckey << endl;

        return eckey;
    }

    string
    Network::Sig2String(ECDSA_SIG * sig) {
        stringstream str;
        str << sig;

        return str.str();
    }

    ECDSA_SIG *
    Network::String2Sig(string str, ECDSA_SIG * sig) {
        uintptr_t temp;
        stringstream ss;
        ss << str;
        ss >> hex >> temp;


        sig = reinterpret_cast<ECDSA_SIG *> (temp);


        return sig;
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

    // =============== Base58 Encoding =============== //
    string
    Network::Base58Encode(const vector<uint8_t> &data) {
        CodecMapping mapping(AlphaMap, Base58Map);
        vector<uint8_t> digits((data.size() * 138 / 100) + 1);
        size_t digitslen = 1;

        for (size_t i = 0; i < data.size(); i++) {
            uint32_t carry = static_cast<uint32_t>(data[i]);
        
            for (size_t j = 0; j < digitslen; j++) {
                carry = carry + static_cast<uint32_t>(digits[j] << 8);
                digits[j] = static_cast<uint8_t>(carry % 58);
                carry /= 58;
            }
        
            for (; carry; carry /= 58) { digits[digitslen++] = static_cast<uint8_t>(carry % 58); }
        }
        string result;

        for (size_t i = 0; i < (data.size() - 1) && !data[i]; i++)  { result.push_back(mapping.BaseMapping[0]); }
        for (size_t i = 0; i < digitslen; i++)                      { result.push_back(mapping.BaseMapping[digits[digitslen - 1 - i]]); } 

        return result;
    }

    // =============== Base58 Decoding =============== //
    vector<uint8_t>
    Network::Base58Decode(const string &data) {
        CodecMapping mapping(AlphaMap, Base58Map);
        vector<uint8_t> result((data.size() * 138 / 100) + 1);
        size_t resultlen = 1;


        for (size_t i = 0; i < data.size(); i++) {
            uint32_t carry = static_cast<uint32_t>(mapping.AlphaMapping[data[i] & 0x7f]);
            for (size_t j = 0; j < resultlen; j++, carry >>= 8) {
                carry += static_cast<uint32_t>(result[j] * 58);
                result[j] = static_cast<uint8_t>(carry);
            }
            for (; carry; carry >>= 8) { result[resultlen++] = static_cast<uint8_t>(carry); }
        }

        result.resize(resultlen);
        for (size_t i = 0; i < (data.size() - 1) && data[i] == mapping.BaseMapping[0]; i++) { result.push_back(0); }

        reverse(result.begin(), result.end());
        return result;
    }

    // =============== Print Bytes =============== //
    void
    Network::PrintBytes(vector<uint8_t> vec) {
        for (size_t i = 0; i < vec.size(); i++) { cout << vec[i]; }
        cout << endl;
    }
 

// ===================================================================================================================================================


    // =============== TEE Class Constructor =============== //
    TEE::TEE(void) {

    }

    // =============== TEE Class Destructor =============== //
    TEE::~TEE(void) {
        NS_LOG_FUNCTION(this);
    }

    // =============== Remote Attestation through TEE =============== //
    vector<uint8_t>
    TEE::RemoteAttestation(vector<uint8_t> nonce, float luck) {
        vector<uint8_t> input;
        vector<uint8_t> proofValue;
        string temp;

        union_data.f = luck;

        // ===== Concatenation of nonce and luck values ===== //
        for (size_t i = 0; i < nonce.size(); i++) { input.push_back(nonce[i]); } 
        for (size_t i = 0; i < SHA::toString(union_data.c).length(); i++) { input.push_back(SHA::toString(union_data.c)[i]); }
        
        temp = network.Base58Encode(input);
        for (size_t i = 0; i < temp.size(); i++) { proofValue.push_back(temp[i]); }

        return proofValue;
    }

    // =============== increment Monotonic Counter value =============== //
    void
    TEE::IncrementMonotonicCounter(void) {
        m_counter += 1;
    }

    // =============== Read Monotonic Counter value =============== //
    int
    TEE::ReadMonotonicCounter(void) {
        return this->m_counter;
    }

    // =============== Get Current Time through TEE =============== //
    Time
    TEE::GetTrustedTime(void) {
        Time currentTime = (Time)Simulator::Now().GetSeconds();
        
        return currentTime;
    }

    // =============== Calculate the luck value included in the proof =============== //
    float
    TEE::LuckInProof(vector<uint8_t> proof) {
        string temp;
        
        vector<uint8_t> data;
        float luck;

        data = network.Base58Decode(network.Bytes2String(proof));

        string hex;
        for (int i = 0; i < 8; i++) { hex += data.at(network.prevHashSize + i); }
        luck = network.Hex2Float(hex);

        return luck;
    }

    // =============== Calculate the nonce value included in the proof =============== //
    vector<uint8_t>
    TEE::NonceInProof(vector<uint8_t> proof) {
        vector<uint8_t> data;
        vector<uint8_t> nonce;

        data = network.Base58Decode(network.Bytes2String(proof));
        
        string hex;
        for(int i = 0; i < network.prevHashSize; i++) { hex += data.at(i); }
        nonce = network.String2Bytes(hex);

        return nonce;
    }

// ===================================================================================================================================================


    // =============== Blockchain Class Constructor =============== //
    Blockchain::Blockchain(void) {

    }

    // =============== Blockchain Class Destructor =============== //
    Blockchain::~Blockchain(void) {
        NS_LOG_FUNCTION(this);
    }
    
    // =============== Add Block to Blockchain =============== //
    void
    Blockchain::AddBlock(Block newBlock) {
        this->blocks.push_back(newBlock);
    }

    // =============== Return the length of the chain =============== //
    int
    Blockchain::GetBlockchainHeight(void) {
        return this->blocks.size();
    }

    // =============== Get Latest Block from Blockchain =============== //
    Block
    Blockchain::GetLatestBlock(void) {
        return this->blocks[this->GetBlockchainHeight() - 1];
    }

    // =============== Serialize Blockchain =============== //
    pair<vector<uint8_t>, int>
    Blockchain::SerializeChain(void) {
        vector<uint8_t> serializedChain;
        int serializedChainSize = 0;

        for (int i = 0; i < this->GetBlockchainHeight(); i++) {
            vector<uint8_t> serializedBlock = this->blocks[i].SerializeBlock().first;
            int serializedBlockSize = this->blocks[i].SerializeBlock().second; 
            serializedChainSize += serializedBlockSize;

            for (int j = 0; j < serializedBlockSize; j++) { serializedChain.push_back(serializedBlock[j]); }
        }

        return make_pair(serializedChain, serializedChainSize);
    }

    // =============== Deserialize Blockchain =============== //
    void
    Blockchain::DeserializeChain(string packet) {
        string blockPacket;

        cout << packet << endl;
        size_t i = 0;
        while(i != packet.size()) {
            blockPacket += packet[i];
            i++;

            if ((packet[i] == '/' && packet[i + 1] == 'b') || (i == packet.size())) {
                Block block;
                block.DeserializeBlock(blockPacket);
                this->AddBlock(block);
                blockPacket.clear();
            }
        }
    }
    
    // =============== Print Blockchain ===============R //
    void
    Blockchain::PrintChain() {
        for (int i = 0; i < this->GetBlockchainHeight(); i++) {
            cout << i << "-th block {";
            this->blocks[i].PrintBlock();
            cout << "}";
            cout << endl;
        }
    }


// ===================================================================================================================================================


    // =============== Block Class Default Constructor =============== // 
    Block::Block(void) {

    }

    // =============== Block Class Constructor =============== //
    Block::Block(vector<uint8_t> prevHash, vector<Tx> txs, vector<uint8_t> proof) {
        this->SetHeader(prevHash, txs);
        this->SetBody(proof);
    }

    // =============== Block Class Destructor =============== // 
    Block::~Block(void) {
        NS_LOG_FUNCTION(this);
    }

    // =============== Set BlockHeader =============== //
    void
    Block::SetHeader(vector<uint8_t> prevHash, vector<Tx> txs) {
        this->header.prevHash       = prevHash;
        this->header.txs            = txs;
    }

    // =============== Set BlockBody =============== //
    void
    Block::SetBody(vector<uint8_t> proof) {
        this->body.proof = proof;
    }

    // =============== Get Block Hash Value =============== //
    vector<uint8_t>
    Block::GetBlockHash() {
        string str;

        vector<uint8_t> headerHash = this->header.GetHeaderHash();
        for (size_t i = 0; i < headerHash.size(); i++)   { str += headerHash[i]; }
        for (size_t i = 0; i < this->body.proof.size(); i++) { str += this->body.proof[i]; }

        string temp;
        SHA512 sha512;
        temp = sha512.hash(str);

        vector<uint8_t> blockHashValue;

        for (int i = 0; i < network.prevHashSize; i++) { blockHashValue.push_back(temp[i]); }

        return blockHashValue;
    }
    
    // =============== Serialize Block =============== //
    pair<vector<uint8_t>, int>
    Block::SerializeBlock(void) {
        vector<uint8_t> serializedPrevHash = this->SerializePrevHash().first;
        vector<uint8_t> serializedTxs = this->SerializeTxs().first;
        vector<uint8_t> serializedProof = this->SerializeProof().first;
        
        int serializedPrevHashSize = this->SerializePrevHash().second;
        int serializedTxsSize = this->SerializeTxs().second;
        int serializedProofSize = this->SerializeProof().second;

        vector<uint8_t> serializedBlock;
        serializedBlock.push_back('/');
        serializedBlock.push_back('b');
        for (int i = 0; i < serializedPrevHashSize; i++)    { serializedBlock.push_back(serializedPrevHash[i]); }
        for (int i = 0; i < serializedTxsSize; i++)         { serializedBlock.push_back(serializedTxs[i]); }
        for (int i = 0; i < serializedProofSize; i++)       { serializedBlock.push_back(serializedProof[i]); }

        return make_pair(serializedBlock, serializedBlock.size());
    }

    // =============== Serialize Previous Block Hash =============== //
    pair<vector<uint8_t>, int>
    Block::SerializePrevHash(void) {
        vector<uint8_t> serializedPrevHash;
        
        serializedPrevHash.push_back('/');
        serializedPrevHash.push_back('h');
        for (size_t i = 0; i < this->header.prevHash.size(); i++) { serializedPrevHash.push_back(this->header.prevHash[i]); }

        return make_pair(serializedPrevHash, serializedPrevHash.size());
    }

    // =============== Serialize Proof =============== //
    pair<vector<uint8_t>, int>
    Block::SerializeProof(void) {
        vector<uint8_t> serializedProof;

        serializedProof.push_back('/');
        serializedProof.push_back('p');
        for (size_t i = 0; i < this->body.proof.size(); i++) { serializedProof.push_back(this->body.proof[i]); }

        return make_pair(serializedProof, serializedProof.size());
    }

    // =============== Serialize Txs =============== //
    pair<vector<uint8_t>, int>
    Block::SerializeTxs(void) {
        vector<uint8_t> serializedTxs;

        for (size_t i = 0; i < this->header.txs.size(); i++) { 
            vector<uint8_t> serializedTx = this->header.txs[i].SerializeTx().first;
            int serializedTxSize = this->header.txs[i].SerializeTx().second;
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
                if      (packet[i + 1] == 'b')  { offset = 'b'; i++;                                               continue; } // 'b'lock
                else if (packet[i + 1] == 'h')  { offset = 'h'; i++;                                               continue; } // prev'h'ash
                else if (packet[i + 1] == 'x')  { offset = 'x'; i++; idx++; Tx tx; this->header.txs.push_back(tx); continue; } // t'x'
                else if (packet[i + 1] == 's')  { offset = 's'; i++;                                               continue; } // 's'ignature
                else if (packet[i + 1] == 'k')  { offset = 'k'; i++;                                               continue; } // ec'k'ey
                else if (packet[i + 1] == 'p')  { offset = 'p'; i++;                                               continue; } // 'p'roof
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
                    this->header.txs[idx].txItem.push_back(packet[i]);
                    continue;

                // 's'ignature
                case 's':
                    this->header.txs[idx].signature.push_back(packet[i]);
                    continue;

                // ec'k'ey
                case 'k':
                    this->header.txs[idx].ecKey.push_back(packet[i]);
                    continue;

                // 'p'roof
                case 'p':
                    this->body.proof.push_back(packet[i]);
                    continue;
            }
        } while(i != packet.size());
    }

    // =============== Print Block =============== //
    void
    Block::PrintBlock(void) {
        cout << "PrevHash: "; network.PrintBytes(this->header.prevHash);
        for (size_t i = 0; i < this->header.txs.size(); i++) { 
            cout << "tx[" << i << "] { " << endl;
            this->header.txs[i].PrintTx(); 
        }
        cout << "}"  << endl;
        cout << "Proof: "; network.PrintBytes(this->body.proof); 
    }

    // =============== BlockHeader Class Constructor =============== //
    Block::BlockHeader::BlockHeader(void) {

    }

    // =============== BlockHeader Class Destructor =============== //
    Block::BlockHeader::~BlockHeader(void) {
        NS_LOG_FUNCTION(this);
    }

    // =============== Get Header Hash Value =============== //
    vector<uint8_t>
    Block::BlockHeader::GetHeaderHash() {
        string str;

        for (int i = 0; i < network.prevHashSize; i++) { str += this->prevHash[i]; }
        for (size_t i = 0; i < this->txs.size(); i++) {
            for (size_t j = 0; j < this->txs[i].txItem.size(); j++) {
                str += this->txs[i].txItem[j];
            }
            for (size_t k = 0; k < this->txs[i].signature.size(); k++) {
                str += this->txs[i].signature[k];
            }
            for (size_t l = 0; l < this->txs[i].ecKey.size(); l++) {
                str += this->txs[i].ecKey[l];
            }
        }

        string temp;
        SHA512 sha512;
        temp = sha512.hash(str);

        vector<uint8_t> headerHashValue;

        for (int i = 0; i < network.prevHashSize; i++) { headerHashValue.push_back(temp[i]); }

        return headerHashValue;
    }
    
    // =============== BlockBody Class Constructor =============== //
    Block::BlockBody::BlockBody(void) {

    }

    // =============== BlockBody Class Destructor =============== //
    Block::BlockBody::~BlockBody(void) {
        NS_LOG_FUNCTION(this);
    }


// ===================================================================================================================================================


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

    // =============== Serialize Transactions =============== //
    pair<vector<uint8_t>, int>
    Tx::SerializeTx(void) {
        vector<uint8_t> serializedTx;

        serializedTx.push_back('/');
        serializedTx.push_back('x');
        for (size_t i = 0; i < this->txItem.size(); i++) { serializedTx.push_back(this->txItem[i]); }

        serializedTx.push_back('/');
        serializedTx.push_back('s');
        for (size_t i = 0; i < this->signature.size(); i++) { serializedTx.push_back(this->signature[i]); }

        serializedTx.push_back('/');
        serializedTx.push_back('k');
        for (size_t i = 0; i < this->ecKey.size(); i++) { serializedTx.push_back(this->ecKey[i]); }

        return make_pair(serializedTx, serializedTx.size());
    }
    // =============== Deserialize Transactions =============== //
    void
    Tx::DeserializeTx(string packet) {
        char offset = NULL;
        int i = -1;
        do {
            i++;
            if (packet[i] == '/') {
                if      (packet[i + 1] == 'x') { offset = 'x'; i++; continue; }
                else if (packet[i + 1] == 's') { offset = 's'; i++; continue; }
                else if (packet[i + 1] == 'k') { offset = 'k'; i++; continue; }
            } else if (packet[i] == '\0') { break; }
    
            switch(offset) {
                // t'x'
                case 'x':
                    this->txItem.push_back(packet[i]);
                    continue;
                
                // 's'ignature
                case 's':
                    this->signature.push_back(packet[i]);
                    continue;

                // ec'k'ey
                case 'k':
                    this->ecKey.push_back(packet[i]);
                    continue;
            }
        } while (i != packet.size());
    }

    // =============== Print Transactions =============== //
    void
    Tx::PrintTx(void) {
        cout << "txItem: ";
        for (size_t i = 0; i < this->txItem.size(); i++)   { cout << this->txItem[i]; }
        cout << endl;
        cout << "sign: ";
        for (size_t i = 0; i < this->signature.size(); i++)     { cout << this->signature[i]; }
        cout << endl;
        cout << "pk: ";
        for (size_t i = 0; i < this->ecKey.size(); i++)       { cout << this->ecKey[i]; }
        cout << endl;
    }


// ===================================================================================================================================================


    // =============== PoLNode Constructor =============== //
    PoLNode::PoLNode(void) {

    }

    // =============== PoLNode Destructor =============== //
    PoLNode::~PoLNode(void) {
        NS_LOG_FUNCTION(this);
    }
   
    // =============== Set TypeId =============== //
    TypeId
    PoLNode::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::PoLNode")                                                      // TypeId of Object
        .SetParent<Application>()                                                                       // Aggregation Object Mechanism
        .SetGroupName("Applications")                                                                   // Group Name that included tid
        .AddConstructor<PoLNode>();                                                                     // Abstract Object Factory Mechanism
        
        return tid;
    }

    // =============== Start Application =============== //
    void
    PoLNode::StartApplication(void) {
        // cout << "PoL ECDSA" << endl;
        // cout << "I love you - yujin-" << endl;

        // ===== Initializing ===== //
        InitializeKey();
        // InitializeMempool();
        InitializeChain();
        m_secureWorker.IncrementMonotonicCounter();

        // ===== Socket Create and Set ===== //           
        if(!m_socket) {         
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");                                 // Set TypeId
            m_socket = Socket::CreateSocket(GetNode(), tid);                                            // Create socket from TypeId
            // m_socket->SetAttribute("RcvBufSize", ns3::UintegerValue(1500000));
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 7071);                   // Create socket address from IP address and port number(7071)
            m_socket->Bind(local);                                                                      // IP and port assignment to Socket
            m_socket->Listen();                                                                         // Set socket to listen
        }

        m_socket->SetRecvCallback(MakeCallback(&PoLNode::HandleRead, this));                            // Set the function to be executed when client receives a packet
        m_socket->SetAllowBroadcast(true);                                                              // Set allow broadcast

        // ===== Socket Connect to Peer Node ===== // 
        vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();                                  // An iterator to get all addresses of peer nodes
        while(iter != m_peersAddresses.end()){                                                          // Connect to all peer nodes
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");                                 // Set TypeId
            Ptr<Socket> socketClient = Socket::CreateSocket(GetNode(), tid);                            // Create client socket from the TypeId
            socketClient->Connect(InetSocketAddress(*iter, 7071));                                      // Connect to node corresponding *iter
            m_peersSockets[*iter] = socketClient;                                                       // Save socketClient to m_peersSocket array
            iter++;                                                                                     // Next peer node
        }


        network.vote.assign(network.numNodes, 0);
        network.delegated.assign(network.numDelegates, -1);
        cout << "nodeId: " << GetNode()->GetId() << endl;
        Simulator::Schedule(Seconds(0), &PoLNode::Vote, this);     
        // Simulator::Schedule(Seconds(0), &PoLNode::NewRound, this);                                      // Schedule NewRound function to generate block
    }

    // =============== Stop Application =============== //
    void
    PoLNode::StopApplication(void) {
        NS_LOG_FUNCTION(this);
    }

    // =============== Function Executed after packet reception =============== //
    void
    PoLNode::HandleRead(Ptr<Socket> socket) {
        cout << "Node[" << GetNode()->GetId() << "] HandleRead Started" << endl;
        
        Ptr<Packet> packet;
        Address from;
        string msg;

        // ===== Packet Processing ===== //
        while(packet = socket->RecvFrom(from)) {
            socket->SendTo(packet, 0, from);
            if (packet->GetSize() == 0) { break; }
            if (InetSocketAddress::IsMatchingType(from)) {
                msg = GetPacketContent(packet, from);
                Tx tx;
                tx.DeserializeTx(msg);
                VerifyTx(tx.txItem, tx.signature, tx.ecKey);
            }
        }

        // m_chain.PrintChain();
        
        // Simulator::Schedule(Seconds(1), &PoLNode::NewRound, this);                                      // Schedule NewRound function to generate block
    }

    // =============== Initialize Key Pair =============== //
    void
    PoLNode::InitializeKey(void) {
        m_ecKey = NULL;
        int curve_nid = NID_secp256k1;
        m_ecKey = EC_KEY_new_by_curve_name(curve_nid);

        EC_KEY_generate_key(m_ecKey);

        m_group = EC_KEY_get0_group(m_ecKey);
        m_privateKey = (BIGNUM *)EC_KEY_get0_private_key(m_ecKey);
        m_ctx = BN_CTX_new();
        m_publicKey = EC_POINT_new(m_group);
        EC_POINT_mul(m_group, m_publicKey, m_privateKey, NULL, NULL, m_ctx);
    }

    // =============== Initialize Memory Pool =============== //
    void
    PoLNode::InitializeMempool(void) {
        for (int i = 0; i < network.numTx; i++) {
            Tx tx = GenerateTx();
            m_memPool.push(tx);
        }
    }


    void
    PoLNode::InitializeChain(void) {
        Block genesisBlock;
        Tx genesisTx = GenerateGenesisTx();

        genesisBlock.header.prevHash.assign(network.prevHashSize, '0');
        genesisBlock.header.txs.push_back(genesisTx);

        /* The proof when nonce and luck values are zero */
        genesisBlock.body.proof = network.String2Bytes("SErCf2nuoEfo4wpinNSaHKepdmsAxxmjav2SMokEwV8V4qg96TdfCGVXEkTLL3RSU5HcssC2cKBCgrCm5vKSYDQBftRHrEndH6dHw6D8ixVDuc39JcbfpxdfKY6nLct1UiSfYGtRcfToQkHKqdR56JFu9ryF8QN8k6oZs59wHLcLiCrqc9751PB4TfqFKeE1SF1PMzTSKEMDfUMZdRPXajA3CX424hzU6qoLqYDsWQMcP5obp3ZaoaEpTo5e84wNFn8fJP");
        
        m_chain.AddBlock(genesisBlock);
    }

    // =============== Broadcast Blockchain =============== //
    void
    PoLNode::SendChain(Blockchain chain) {
        // cout << "Node[" << GetNode()->GetId() << "] SendChain Started (Chain Broadcast)" << endl;

        int dataSize = chain.SerializeChain().second;
        
        uint8_t * data = network.Bytes2Packet(chain.SerializeChain().first, dataSize);
        cout << "dataSize: " << dataSize << endl;

        Ptr<Packet> p;
        p = Create<Packet>(data, dataSize); // 65507

        vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();               
        while(iter != m_peersAddresses.end()) {                       
            Ptr<Socket> socketClient = m_peersSockets[*iter];                        
            Simulator::Schedule(Seconds(0), &PoLNode::SendPacket, this, socketClient, p);
            iter++;
        }

        // Simulator::Schedule(Seconds(1), &PoLNode::NewRound, this);
    }


    // =============== Broadcast Transaction =============== //
    void
    PoLNode::SendTx(Tx tx) {
        cout << "Node[" << GetNode()->GetId() << "] SendTx Started (Tx Broadcast)" << endl << endl;

        int dataSize = tx.SerializeTx().second;
        uint8_t * data = network.Bytes2Packet(tx.SerializeTx().first, dataSize);

        Ptr<Packet> p;
        p = Create<Packet>(data, dataSize);

        vector<Ipv4Address>::iterator iter = m_peersAddresses.begin();               
        while(iter != m_peersAddresses.end()) {                                      
            Ptr<Socket> socketClient = m_peersSockets[*iter];                        
            Simulator::Schedule(Seconds(0), &PoLNode::SendPacket, this, socketClient, p);
            iter++;                  
        }

        // cout << "Tx Broadcast is done" << endl;

        // Simulator::Schedule(Seconds(1), &PoLNode::NewRound, this);
    }

    // =============== Vote =============== //
    void
    PoLNode::Vote(void) {
        cout << "Node[" << GetNode()->GetId() << "] Vote Started" << endl;
        
        float luck = GetRandomLuck();
        Tx vote;
        string voteItem = "766f7465";                                                           // "vote"
        vote.txItem = network.String2Bytes(voteItem);

        vote.signature = network.String2Bytes(SignTx(vote.txItem, m_ecKey));
        vote.ecKey = network.String2Bytes(network.Eckey2String(m_ecKey));

        int voteId = dis(gen) % network.numNodes;
        network.vote[voteId] += luck;

        if (GetNode()->GetId() == network.numNodes - 1) {
            SelectDelegates();
        }

        Simulator::Schedule(Seconds(0), &PoLNode::SendTx, this, vote);
    }

    // =============== Select Delegates =============== //
    void
    PoLNode::SelectDelegates(void) {
        cout << "Node[" << GetNode()->GetId() << "] SelecetDelegates Started" << endl;
        int nDelegates = 0;
        
        cout << "[vote]" << endl;
        for (int i = 0; i < network.vote.size(); i++) { cout << network.vote[i] << ", "; }
        cout << endl;

        for (int i = 0; i < network.numDelegates; i++) {
            int idx = 0;
            float temp = -1;

            for (int j = 0; j < network.vote.size(); j++) {
                if(temp < network.vote[j]) { 
                    if (isDelegated(j)) { continue; }

                    temp = network.vote[j];
                    idx = j; 
                }
            }

            network.delegated[nDelegates] = idx;
            nDelegates++;
        }

        cout << "delegated selected" << endl;
        cout << "[delegated]" << endl;
        for (int i = 0; i < network.delegated.size(); i++) { cout << network.delegated[i] << ", "; }
        cout << endl;
    }


    // =============== already delegated =============== //
    bool
    PoLNode::isDelegated(int idx) {
        for (int i = 0; i < network.delegated.size(); i++) {
            if(network.delegated[i] == idx) { return 1;}
        }

        return 0;
    }
    
    // =============== Broadcast Packet =============== //
    void
    PoLNode::SendPacket(Ptr<Socket> socketClient, Ptr<Packet> p) {
        socketClient->Send(p);
    }

    // =============== Get Packet Content =============== //
    string
    PoLNode::GetPacketContent(Ptr<Packet> packet, Address from) {
        char *packetInfo = new char[packet->GetSize() + 1];
        ostringstream totalStream;
        packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo), packet->GetSize());
        packetInfo[packet->GetSize()] = '\0';
        totalStream << m_bufferedData[from] << packetInfo;
        string totalReceivedData(totalStream.str());

        return totalReceivedData;
    }

    // =============== 무작?�� luck�? ?��?�� =============== //
    float
    PoLNode::GetRandomLuck(void) {
        float l;
        l = (float)dis(gen) / dis.max();                           // luck ?�� [0, 1)

        return l;
    }

    // =============== NewRound to Start New Round =============== //
    void
    PoLNode::NewRound(void) {
        // cout << "Node[" << GetNode()->GetId() << "] NewRound Started" << endl;
        round++;

        int act = dis(gen);
        // if(act % 2 == 0) {
            Block roundBlock;
            roundBlock = m_chain.GetLatestBlock();
                                                                                     
            Simulator::Schedule(Seconds(0), &PoLNode::PoLRound, this, roundBlock);
        // }

        // else {
        //     Tx newTx;
        //     newTx.GenerateTx();

        //     Simulator::Schedule(Seconds(0), &PoLNode::SendTx, this, newTx);
        // }

    }

    // =============== PoLRound to Prepare the TEE =============== //
    void
    PoLNode::PoLRound(Block block) {
        // cout << "Node[" << GetNode()->GetId() << "] PoLRound Start" << endl;

        m_roundBlock = block;
        m_roundTime = m_secureWorker.GetTrustedTime();
        vector<Tx> newTxs = GetPendingTxs();

        Simulator::Schedule(Seconds(0), &PoLNode::Commit, this, newTxs);
    }

    // =============== PoLMine for Block Generation =============== //
    void
    PoLNode::PoLMine(Block::BlockHeader header, Block previousBlock) {
        // cout << "Node[" << GetNode()->GetId() << "] PoLMine Started" << endl;

        Time now = m_secureWorker.GetTrustedTime();
        int newCounter = m_secureWorker.ReadMonotonicCounter();

        // ===== Validation ===== //
        if (header.prevHash != previousBlock.GetBlockHash()
            || previousBlock.header.prevHash != m_roundBlock.header.prevHash
            || now < m_roundTime + (Time)ROUND_TIME
            || m_secureWorker.m_counter != newCounter) {
                NS_LOG_INFO("Validation Failed in PoLMine");
                return;
            } // else { NS_LOG_INFO("Validation Success in PoLMine"); }

        float luck = GetRandomLuck();
        // sleep(GetWaitingTime(luck));
        
        vector<uint8_t> nonce = header.GetHeaderHash();
        vector<uint8_t> proof = m_secureWorker.RemoteAttestation(nonce, luck);

        Block newBlock(header.prevHash, header.txs, proof);
        m_chain.AddBlock(newBlock);

        Simulator::Schedule(Seconds(0), &PoLNode::SendChain, this, m_chain);
    }

    // =============== Append New Block to Blockchain =============== //
    void
    PoLNode::Commit(vector<Tx> newTxs) {
        // cout << "Node[" << GetNode()->GetId() << "] Commit Started" << endl;

        Block previousBlock = m_chain.GetLatestBlock();
        Block::BlockHeader header;
        header.prevHash = previousBlock.GetBlockHash();
        header.txs = newTxs;

        /* Schedule is used to give delay(ROUND_TIME) before PoLMine execution. */
        /* Block is added to the chain in the commit function. but, Schedule cannot return, so AddBlock is executed within PoLMine. */
        Simulator::Schedule(Seconds(ROUND_TIME), &PoLNode::PoLMine, this, header, previousBlock);
    }

    // =============== Blockchain Validation =============== //
    void
    PoLNode::Valid(Blockchain chain) {
        for (int i = 1; i < chain.GetBlockchainHeight() - 1; i++) {
            if (chain.blocks[i].header.prevHash != chain.blocks[i - 1].GetBlockHash()
            || m_secureWorker.NonceInProof(chain.blocks[i].body.proof) != chain.blocks[i].header.GetHeaderHash()
            || chain.blocks[i].body.proof != 
                m_secureWorker.RemoteAttestation(m_secureWorker.NonceInProof(chain.blocks[i].body.proof), m_secureWorker.LuckInProof(chain.blocks[i].body.proof))) {
                NS_LOG_INFO("VALID failed");
                return;
            }
        }
        // NS_LOG_INFO("VALID successed");
    }

    // =============== Summarization luck values in blockchain =============== //
    float
    PoLNode::Luck(Blockchain chain) {
        float chainLuck = 0;


        for (int i = 0; i < chain.GetBlockchainHeight(); i++) {
            chainLuck += m_secureWorker.LuckInProof(chain.blocks[i].body.proof);
        }

        return chainLuck;
    }

    // =============== Calculate Wait Time Corresponding to Luck Value =============== //
    float
    PoLNode::GetWaitingTime(float luck) {
        float waitingTime = (1 - luck) * ROUND_TIME / 2 + 1;
        NS_LOG_INFO("Waiting for " << waitingTime << "s");

        return waitingTime;
    }

    // =============== Get Pending Transactions from Memory Pool =============== //
    vector<Tx>
    PoLNode::GetPendingTxs(void) {
        
        vector<Tx> newTxs;
        int txStorage = network.txStorage;

        while(true) {
            if (m_memPool.empty()) { 
                // cout << "Memory Pool is empty" << endl; 
                break;
            }

            Tx tx = m_memPool.front();
            int txSize = tx.txItem.size() + tx.signature.size() + tx.ecKey.size();
            if (txSize <= txStorage) {
                m_memPool.pop();
                newTxs.push_back(tx);
                txStorage = txStorage - txSize;
            } else { break; }
        }

       return newTxs;
    }

    // =============== Add Pending Transaction to Memory Pool =============== //
    void
    PoLNode::AddPendingTx(Tx pendingTx) {
        m_memPool.push(pendingTx);
    }

    // =============== Generate Genesis Transaction =============== //
    Tx
    PoLNode::GenerateGenesisTx(void) {
        uint8_t arr[11];
        network.String2Array("43727970746f4372616674", arr);
        vector<uint8_t> genesisTxItem = network.Array2Bytes(arr, 11);

        Tx genesisTx;
        genesisTx.txItem = genesisTxItem;
        genesisTx.signature = network.String2Bytes(SignTx(genesisTx.txItem, m_ecKey));
        genesisTx.ecKey = network.String2Bytes(network.Eckey2String(m_ecKey));

        return genesisTx;
    }

    // =============== Generate Transaction =============== //
    Tx
    PoLNode::GenerateTx(void) {
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

        tx.signature = network.String2Bytes(SignTx(tx.txItem, m_ecKey));
        tx.ecKey = network.String2Bytes(network.Eckey2String(m_ecKey));
        
        return tx;
    }

    string
    PoLNode::SignTx(vector<uint8_t> txItem, EC_KEY * ec_key) {
        // TODO: 이 부분 Bytes2UChar로 구현
        string str_hash = network.Bytes2String(txItem);
        unsigned char hash[str_hash.length()+1];
        for(int i = 0; i < sizeof(hash); i++) {
            hash[i] = str_hash[i];
        }

        int size = txItem.size() / 2;
        ECDSA_SIG *signature = ECDSA_do_sign(hash, size, ec_key);
        if (NULL == signature) { cout << "Failed" << endl; }
        else { cout << "Succeed" << endl; }

        cout << "signature address in signTx: " << signature << endl;
        cout << "ec_key address in signTx: " << ec_key << endl;

        string sig = network.Sig2String(signature);

        return sig;
    }

    bool
    PoLNode::VerifyTxs(Blockchain chain) {
        for (int i = 0; i < chain.GetBlockchainHeight(); i++) {
            for (int j = 0; j < chain.blocks[i].header.txs.size(); j++) {
                Tx tx = chain.blocks[i].header.txs[j];
                VerifyTx(tx.txItem, tx.signature, tx.ecKey);
            }
        }

        return true;
    }

    bool
    PoLNode::VerifyTx(vector<uint8_t> txItem, vector<uint8_t> sign, vector<uint8_t> ecKey) {
        uintptr_t temp;
        stringstream ss;
        ss << network.Bytes2String(sign);
        ss >> hex >> temp;

        ECDSA_SIG * signature = NULL;
        signature = reinterpret_cast<ECDSA_SIG *> (temp);

        uintptr_t temp2;
        stringstream ss2;
        ss2 << network.Bytes2String(ecKey);
        ss2 >> hex >> temp2;

        EC_KEY * ec_key = NULL;
        ec_key = reinterpret_cast<EC_KEY *> (temp2);

        
        string str_hash = network.Bytes2String(txItem);
        unsigned char hash[str_hash.length()+1];
        for(int i = 0; i < sizeof(hash); i++) {
            hash[i] = str_hash[i];
        }

        cout << "signature address in verifyTx: " << signature << endl;
        network.PrintBytes(sign);
        cout << "ec_key address in verifyTx: " << ec_key << endl;
        network.PrintBytes(ecKey);

        int size = txItem.size() / 2;
        int verify_status = ECDSA_do_verify(hash, size, signature, ec_key);
        const int verify_success = 1;

        if (verify_success != verify_status) { 
            cout << "VerifyTx Failed" << endl;
            return OQS_ERROR;
        }
        else { 
            cout << "VerifyTx Successed" << endl; 
            return OQS_SUCCESS;
        }
    }

// ===================================================================================================================================================
}