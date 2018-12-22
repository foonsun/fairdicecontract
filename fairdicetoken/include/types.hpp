#ifndef FAIRDICEGAME_TYPES_HPP
#define FAIRDICEGAME_TYPES_HPP

using namespace eosio;
using namespace std;

// @abi table accounts i64
struct account {
    asset    balance;

    uint64_t primary_key()const { return balance.symbol.name(); }
};

struct currency_stats {
    asset          supply;
    asset          max_supply;
    account_name   issuer;

    uint64_t primary_key()const { return supply.symbol.name(); }
};

// @abi table allaccounts i64
struct allaccount {
    account_name  account;
    asset    balance;

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

typedef eosio::multi_index<N(accounts), account> accounts;
typedef eosio::multi_index<N(stat), currency_stats> stats;
typedef eosio::multi_index<N(allaccounts), allaccount> allaccounts;
typedef eosio::multi_index<N(icoinfos), icoinfo> icoinfos;

#endif //FAIRDICEGAME_TYPES_HPP
