#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/types.hpp>
#include <iostream>
using namespace eosio;
using namespace std;

#define FAIR_DICE_GAME N(bobinggame11)

struct st_bet {
    uint64_t id;
    account_name player;
    account_name referrer;
    asset amount;
    /* uint8_t roll_under; */
    checksum256 seed_hash;
    checksum160 user_seed_hash;
    uint64_t created_at;
};

struct st_result {
    uint64_t bet_id;
    account_name player;
    account_name referrer;
    asset amount;
    /* uint8_t roll_under; */
    uint8_t random_roll_1;
    uint8_t random_roll_2;
    uint8_t random_roll_3;
    uint8_t random_roll_4;
    uint8_t random_roll_5;
    uint8_t random_roll_6;
    checksum256 seed;
    checksum256 seed_hash;
    checksum160 user_seed_hash;
    asset payout;
};

class fairdicelogs : public contract {
   public:
    fairdicelogs(account_name self) : contract(self){};

    // @abi action
    void result(st_result result);
};

EOSIO_ABI(fairdicelogs, (result));
