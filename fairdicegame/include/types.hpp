#include "utils.hpp"

#define EOS_SYMBOL S(4, EOS)
#define DICE_SYMBOL S(4, EDICE)
#define REVEALER N(dicereveal11)
#define DICETOKEN N(gooddicebank)
#define DEV N(gooddicedev1)
#define DIVIDEND N(gooddicediv1)
#define DICESUPPLY (1000000000)
#define MINEINTERVAL (100000000)
#define NEXTMINE (400000000)
#define EOSPERDICE (10)

static const string PUB_KEY =
    "EOS79dRVEJx8t8DKqgcS68six32XVoTzwimG2xPmVv4z3dab987ro";

// @abi table bets i64
struct st_bet {
    uint64_t id;
    account_name player;
    account_name referrer;
    extended_asset amount;
    uint8_t roll_under;
    checksum256 seed_hash;
    checksum160 user_seed_hash;
    uint64_t created_at;
    uint64_t expiration;

    uint64_t primary_key() const { return id; }
    uint64_t by_expiration() const { return expiration; }
};

struct st_result {
    uint64_t bet_id;
    account_name player;
    account_name referrer;
    extended_asset amount;
    uint8_t roll_under;
    checksum256 seed;
    checksum256 seed_hash;
    checksum160 user_seed_hash;
    asset payout;
};

// @abi table hash i64
struct st_hash {
    checksum256 hash;
    uint64_t expiration;
    uint64_t primary_key() const { return uint64_hash(hash); }

    uint64_t by_expiration() const { return expiration; }
};

// @abi table fundpool i64
struct st_fund_pool {
    asset locked;
};

// @abi table global i64
struct st_global {
    uint64_t current_id;
    double eosperdice;
    uint64_t nexthalve;
    uint64_t initStatu;
};
// @abi table tokens i64
struct st_tokens
{
    account_name contract; // 合约账号
    symbol_type symbol;    // 代币名称
    uint64_t minAmout;     //最小允许投注的值
    uint64_t primary_key() const { return contract + symbol; }
};

// @abi table users1 i64
struct st_user1
{
    asset amount = asset(0, EOS_SYMBOL);
    uint32_t count = 0;
};

// @abi table users i64
struct st_user
{
    account_name owner;
    asset amount;
    uint32_t count;
    uint64_t primary_key() const { return owner; }
};

typedef singleton<N(users1), st_user1> tb_users1;
typedef multi_index<N(tokens), st_tokens> tb_tokens;
typedef multi_index<
        N(bets),
        st_bet,
        indexed_by<N(by_expiration),
                   const_mem_fun<st_bet, uint64_t, &st_bet::by_expiration>>>
        tb_bets;
typedef singleton<N(fundpool), st_fund_pool> tb_fund_pool;
typedef singleton<N(global), st_global> tb_global;
typedef multi_index<
    N(hash),
    st_hash,
    indexed_by<N(by_expiration),
               const_mem_fun<st_hash, uint64_t, &st_hash::by_expiration>>>
    tb_hash;
