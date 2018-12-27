#ifndef FAIRDICEGAME_TYPES_HPP
#define FAIRDICEGAME_TYPES_HPP

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/action.h>
#include <eosiolib/singleton.hpp>

#define TOKEN_SYMBOL    S(4,MMT)
#define PHASE_ONE       1000000000

#define TEAM_UNLOCK (60)
#define ICO_UNLOCK (40)
#define TUIGUANG_UNLOCK (10)

#define MINE_ACCOUNT_GROUP (0)
#define TEAM_ACCOUNT_GROUP (1)
#define ICO_ACCOUNT_GROUP (2)
#define TUIGUANG_ACCOUNT_GROUP (3)

using namespace eosio;
using namespace std;

// @abi table accounts i64
struct account {
    asset    balance;
    asset    lock_balance;
    uint64_t primary_key()const { return balance.symbol.name(); }
};

// @abi table stat i64
struct currencystat {
    asset          supply;
    asset          max_supply;
    account_name   issuer;

    uint64_t primary_key()const { return supply.symbol.name(); }
};

// @abi table allaccounts i64
struct allaccount {
    account_name  account;
    asset    balance;
    asset    lock_balance;
    uint64_t account_group;

    uint64_t primary_key()const { return account; }
};

// @abi table icoinfos i64
struct icoinfo{
    uint64_t id;
    account_name account;
    asset quant;
    time_point_sec time;

    uint64_t primary_key()const { return id; }
};

// @abi table globals i64
struct global {
    asset mine;
};

typedef eosio::multi_index<N(accounts), account> accounts;
typedef eosio::multi_index<N(stat), currencystat> stats;
typedef eosio::multi_index<N(allaccounts), allaccount> allaccounts;
typedef eosio::multi_index<N(icoinfos), icoinfo> icoinfos;
typedef eosio::singleton<N(globals), global> globals;

#endif //FAIRDICEGAME_TYPES_HPP
